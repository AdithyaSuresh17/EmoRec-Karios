/* =============================================================================
   VGG Emotion Inference - Arduino Nano 33 BLE Sense
   
   Architecture: EmotionCNN_RegV2 (TinyVGG-style)
     Input:  (1, 14, 125) — float32
     Output: (4,)         — [distracted, focused, relaxed, stressed]
   
   Data flow:
     UNO → SoftwareSerial(8,9) → BLE Sense → parse packet → circular buffer
     → extract window → z-score normalise → TFLite inference → Serial output
   
   Incoming packet format from UNO (77 bytes total):
     [0x00]         : start byte 0xAA
     [0x01..0x24]   : imuData struct #1  (9 × float32 = 36 bytes)
                        ax, ay, az, r, p, y, avx, avy, avz
     [0x25..0x48]   : imuData struct #2  (9 × float32 = 36 bytes)
                        ax, ay, az, r, p, y, avx, avy, avz
     [0x49..0x4A]   : EMG1 (uint16, 2 bytes)
     [0x4B..0x4C]   : EMG2 (uint16, 2 bytes)
   
   Feature extraction (14 channels, matching training column order):
     ch 0-5  : IMU1 → ax1, ay1, az1, avx1, avy1, avz1   (skip r,p,y at offsets 3,4,5)
     ch 6-11 : IMU2 → ax2, ay2, az2, avx2, avy2, avz2
     ch 12   : EMG1 (raw uint16 cast to float)
     ch 13   : EMG2 (raw uint16 cast to float)

   Z-score normalisation per window per channel — matches training pipeline exactly.
   ============================================================================= */

#include <Chirale_TensorFlowLite.h>
#include "vgg_model.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

// ---------------------------------------------------------------------------
// Model / windowing constants
// ---------------------------------------------------------------------------
#define NUM_CHANNELS   14
#define WINDOW_SIZE   125
#define STEP_SIZE      25
#define NUM_EMOTIONS    4

// ---------------------------------------------------------------------------
// Serial link from UNO
// ---------------------------------------------------------------------------
// The UNO sends on its SoftwareSerial TX (pin 9) → connect to BLE Sense RX pin
// Use hardware Serial1 on the BLE Sense (pins 0/1) for reliable high-speed comms.
// Wire: UNO pin 9 (TX) → BLE Sense pin 0 (RX)
#define SENSE_SERIAL  Serial1
#define LINK_BAUD     38400

// ---------------------------------------------------------------------------
// Packet geometry
// ---------------------------------------------------------------------------
#define IMU_FLOATS        9          // ax,ay,az,r,p,y,avx,avy,avz
#define IMU_BYTES         (IMU_FLOATS * 4)   // 36 bytes
#define EMG_BYTES         2                  // uint16
#define PACKET_PAYLOAD    (IMU_BYTES * 2 + EMG_BYTES * 2)  // 76 bytes
#define PACKET_TOTAL      (1 + PACKET_PAYLOAD)              // 77 bytes (inc. start byte)
#define START_BYTE        0xAA

// Float offsets within one imuData struct (index into float array):
//   0=ax  1=ay  2=az  3=r  4=p  5=y  6=avx  7=avy  8=avz
#define IDX_AX   0
#define IDX_AY   1
#define IDX_AZ   2
// skip 3,4,5 (roll, pitch, yaw — not used in training)
#define IDX_AVX  6
#define IDX_AVY  7
#define IDX_AVZ  8

// ---------------------------------------------------------------------------
// Circular buffer  (2 × WINDOW_SIZE so we always have a full window available)
// ---------------------------------------------------------------------------
#define BUFFER_SIZE  (WINDOW_SIZE * 2)

float  sensor_buffer[NUM_CHANNELS][BUFFER_SIZE];
int    write_idx    = 0;
int    total_samples = 0;

// Flat window passed to TFLite: layout [ch * WINDOW_SIZE + t]
float  window_flat[NUM_CHANNELS * WINDOW_SIZE];

// ---------------------------------------------------------------------------
// Emotion labels — alphabetical order matches sklearn LabelEncoder default
// (distracted=0, focused=1, relaxed=2, stressed=3)
// ---------------------------------------------------------------------------
const char* EMOTION_LABELS[NUM_EMOTIONS] = {
  "distracted", "focused", "relaxed", "stressed"
};

// ---------------------------------------------------------------------------
// TFLite globals
// ---------------------------------------------------------------------------
const tflite::Model*      tfl_model      = nullptr;
tflite::MicroInterpreter* interpreter    = nullptr;
TfLiteTensor*             tfl_input      = nullptr;
TfLiteTensor*             tfl_output     = nullptr;

// Tensor arena — VGG with hidden=10, input (1,14,125):
//   Block1 output: (10, 62) → 620 floats
//   Block2 output: (10, 31) → 310 floats
//   Weights are in flash so arena only needs activations + a little overhead.
//   Start at 32 KB; increase by 4 KB increments if AllocateTensors() fails.
constexpr int kTensorArenaSize = 32 * 1024;
alignas(16) uint8_t tensor_arena[kTensorArenaSize];

// ---------------------------------------------------------------------------
// Raw packet buffer
// ---------------------------------------------------------------------------
uint8_t pkt_buf[PACKET_TOTAL];

// ===========================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println("=== VGG Emotion Inference — BLE Sense ===");

  // Open hardware serial link to UNO
  SENSE_SERIAL.begin(LINK_BAUD);

  // --- TFLite initialisation ---
  tfl_model = tflite::GetModel(VGG_Emotion_Model_float32_tflite);
  if (tfl_model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("ERROR: model schema version mismatch!");
    while (true) {}
  }

  static tflite::AllOpsResolver resolver;

  static tflite::MicroInterpreter static_interpreter(
      tfl_model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  TfLiteStatus alloc_status = interpreter->AllocateTensors();
  if (alloc_status != kTfLiteOk) {
    Serial.println("ERROR: AllocateTensors() failed — increase kTensorArenaSize");
    while (true) {}
  }

  tfl_input  = interpreter->input(0);
  tfl_output = interpreter->output(0);

  // Sanity-check tensor shapes
  Serial.print("Input  tensor bytes : "); Serial.println(tfl_input->bytes);
  Serial.print("Output tensor bytes : "); Serial.println(tfl_output->bytes);
  // Expected: input = 14*125*4 = 7000 bytes, output = 4*4 = 16 bytes

  Serial.println("Ready. Waiting for data from UNO...");
  Serial.println("FORMAT: millis,emotion,confidence,class_idx");
}

// ===========================================================================
void loop() {
  if (read_packet()) {
    parse_and_buffer_packet();

    // Once we have enough samples, infer every STEP_SIZE samples
    if (total_samples >= WINDOW_SIZE && (total_samples % STEP_SIZE == 0)) {
      run_inference();
    }
  }
}

// ===========================================================================
// Read exactly one framed packet from Serial1.
// Returns true when a valid packet has been placed in pkt_buf[].
// Strategy: scan for 0xAA start byte, then read remaining bytes.
// ===========================================================================
bool read_packet() {
  // Wait for start byte
  while (SENSE_SERIAL.available()) {
    uint8_t b = SENSE_SERIAL.read();
    if (b != START_BYTE) continue;

    // Start byte found — read the rest of the packet
    pkt_buf[0] = START_BYTE;
    int bytes_read = 1;
    unsigned long deadline = millis() + 50;  // 50 ms timeout

    while (bytes_read < PACKET_TOTAL && millis() < deadline) {
      if (SENSE_SERIAL.available()) {
        pkt_buf[bytes_read++] = SENSE_SERIAL.read();
      }
    }

    if (bytes_read == PACKET_TOTAL) return true;

    // Incomplete packet — discard and keep scanning
    Serial.print("WARN: incomplete packet (got ");
    Serial.print(bytes_read); Serial.println(" bytes)");
  }
  return false;
}

// ===========================================================================
// Parse pkt_buf → extract 14 features → push into circular buffer
// ===========================================================================
void parse_and_buffer_packet() {
  // --- IMU structs start at byte 1 ---
  float imu1[IMU_FLOATS], imu2[IMU_FLOATS];
  memcpy(imu1, pkt_buf + 1,               IMU_BYTES);
  memcpy(imu2, pkt_buf + 1 + IMU_BYTES,   IMU_BYTES);

  // --- EMG values follow both IMU structs ---
  uint16_t emg1_raw, emg2_raw;
  memcpy(&emg1_raw, pkt_buf + 1 + IMU_BYTES * 2,              EMG_BYTES);
  memcpy(&emg2_raw, pkt_buf + 1 + IMU_BYTES * 2 + EMG_BYTES,  EMG_BYTES);

  // --- Build 14-element feature vector (matches training column order) ---
  //   ax1, ay1, az1, gx1(=avx1), gy1(=avy1), gz1(=avz1),
  //   ax2, ay2, az2, gx2(=avx2), gy2(=avy2), gz2(=avz2),
  //   emg1, emg2
  float features[NUM_CHANNELS] = {
    imu1[IDX_AX],  imu1[IDX_AY],  imu1[IDX_AZ],
    imu1[IDX_AVX], imu1[IDX_AVY], imu1[IDX_AVZ],
    imu2[IDX_AX],  imu2[IDX_AY],  imu2[IDX_AZ],
    imu2[IDX_AVX], imu2[IDX_AVY], imu2[IDX_AVZ],
    (float)emg1_raw,
    (float)emg2_raw
  };

  // --- Push into circular buffer ---
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    sensor_buffer[ch][write_idx] = features[ch];
  }
  write_idx = (write_idx + 1) % BUFFER_SIZE;
  total_samples++;
}

// ===========================================================================
// Extract the most recent WINDOW_SIZE samples, z-score normalise, infer
// ===========================================================================
void run_inference() {
  // --- Extract latest WINDOW_SIZE samples from circular buffer ---
  int start_idx = (write_idx - WINDOW_SIZE + BUFFER_SIZE) % BUFFER_SIZE;

  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    for (int t = 0; t < WINDOW_SIZE; t++) {
      int buf_idx = (start_idx + t) % BUFFER_SIZE;
      window_flat[ch * WINDOW_SIZE + t] = sensor_buffer[ch][buf_idx];
    }
  }

  // --- Per-window, per-channel z-score (matches training: axis=0, keepdims) ---
  //   window_norm = (window - mean) / (std + 1e-8)
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    // Mean
    float mean = 0.0f;
    for (int t = 0; t < WINDOW_SIZE; t++) {
      mean += window_flat[ch * WINDOW_SIZE + t];
    }
    mean /= WINDOW_SIZE;

    // Population std  (training uses np.std which is population, ddof=0)
    float var = 0.0f;
    for (int t = 0; t < WINDOW_SIZE; t++) {
      float diff = window_flat[ch * WINDOW_SIZE + t] - mean;
      var += diff * diff;
    }
    float std_dev = sqrtf(var / WINDOW_SIZE);

    // Normalise
    float denom = std_dev + 1e-8f;
    for (int t = 0; t < WINDOW_SIZE; t++) {
      int idx = ch * WINDOW_SIZE + t;
      window_flat[idx] = (window_flat[idx] - mean) / denom;
    }
  }

  // --- Copy into TFLite input tensor ---
  // Model expects shape (1, 14, 125) — channel-first, same as window_flat layout
  memcpy(tfl_input->data.f, window_flat, NUM_CHANNELS * WINDOW_SIZE * sizeof(float));

  // --- Run inference ---
  TfLiteStatus status = interpreter->Invoke();
  if (status != kTfLiteOk) {
    Serial.println("ERROR: Invoke() failed");
    return;
  }

  // --- Argmax over 4 logits / probabilities ---
  float* out = tfl_output->data.f;
  int    best_class = 0;
  float  best_score = out[0];
  for (int i = 1; i < NUM_EMOTIONS; i++) {
    if (out[i] > best_score) {
      best_score = out[i];
      best_class = i;
    }
  }

  // --- Output: millis, label, score, class_idx ---
  Serial.print(millis());
  Serial.print(",");
  Serial.print(EMOTION_LABELS[best_class]);
  Serial.print(",");
  Serial.print(best_score, 4);
  Serial.print(",");
  Serial.println(best_class);
}
