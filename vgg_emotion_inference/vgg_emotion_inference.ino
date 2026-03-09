/* =============================================================================
   VGG Emotion Inference - Arduino Nano 33 BLE Sense
   
   Replicates Python inference pipeline exactly:
     1. Circular buffer accumulates raw samples [N, 14]
     2. Every STEP_SIZE new samples, extract last WINDOW_SIZE rows → [125, 14]
     3. Per-window, per-channel z-score normalisation (axis=0, keepdims) 
     4. Feed into TFLite model as (1, 125, 14) — TIME-FIRST (matches TFLite model)
     5. Argmax over 4 output logits → label + confidence
   
   NOTE: Python uses torch input (N, 14, 125) channel-first for PyTorch,
         but the TFLite model was exported with input shape (1, 125, 14) time-first.
         This C code feeds (1, 125, 14) directly — no transpose needed.

   Incoming packet format from UNO (77 bytes total):
     [0x00]         : start byte 0xAA
     [0x01..0x24]   : imuData struct #1  (9 × float32 = 36 bytes)
                        ax, ay, az, r, p, y, avx, avy, avz
     [0x25..0x48]   : imuData struct #2  (9 × float32 = 36 bytes)
                        ax, ay, az, r, p, y, avx, avy, avz
     [0x49..0x4A]   : EMG1 (uint16, 2 bytes)
     [0x4B..0x4C]   : EMG2 (uint16, 2 bytes)

   14 channels (matching training column order):
     ch 0-5  : IMU1 → ax1, ay1, az1, gx1, gy1, gz1   (skip r,p,y)
     ch 6-11 : IMU2 → ax2, ay2, az2, gx2, gy2, gz2   (skip r,p,y)
     ch 12   : EMG1
     ch 13   : EMG2

   Sampling: ~28.5Hz (35ms/sample). Window: 125 samples. Step: 25 samples.
   ============================================================================= */

#include <Chirale_TensorFlowLite.h>
#include "emotion_model_int8.h"   // ← your xxd-generated header (full_integer_quant model)

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

// ---------------------------------------------------------------------------
// Constants — must match training exactly
// ---------------------------------------------------------------------------
#define NUM_CHANNELS   14
#define WINDOW_SIZE   125
#define STEP_SIZE      25     // run inference every 25 new samples (matches Python)
#define NUM_EMOTIONS    4

#define SAMPLE_INTERVAL_MS  35   // ~28.5Hz — match your training logging rate

// ---------------------------------------------------------------------------
// Serial link from UNO
// ---------------------------------------------------------------------------
#define SENSE_SERIAL  Serial1
#define LINK_BAUD     38400

// ---------------------------------------------------------------------------
// Packet geometry
// ---------------------------------------------------------------------------
#define IMU_FLOATS      9
#define IMU_BYTES       (IMU_FLOATS * 4)   // 36 bytes
#define EMG_BYTES       2
#define PACKET_PAYLOAD  (IMU_BYTES * 2 + EMG_BYTES * 2)   // 76 bytes
#define PACKET_TOTAL    (1 + PACKET_PAYLOAD)               // 77 bytes
#define START_BYTE      0xAA

// Float offsets within imuData struct
#define IDX_AX   0
#define IDX_AY   1
#define IDX_AZ   2
// 3,4,5 = roll,pitch,yaw — skipped (not in training)
#define IDX_AVX  6
#define IDX_AVY  7
#define IDX_AVZ  8

// ---------------------------------------------------------------------------
// Circular buffer
// BUFFER_SIZE must be >= WINDOW_SIZE. Use 2× so we always have a full window.
// Layout: sensor_buffer[sample_index][channel]  → matches Python [N, 14]
// ---------------------------------------------------------------------------
#define BUFFER_SIZE  (WINDOW_SIZE * 2)

float  sensor_buffer[BUFFER_SIZE][NUM_CHANNELS];   // [N, 14] — time-major
int    write_idx                    = 0;
int    total_samples                = 0;
int    samples_since_last_inference = 0;
bool   buffer_primed                = false;
unsigned long last_sample_ms        = 0;

// Flat window for TFLite: layout [t * NUM_CHANNELS + ch]  → shape (1, 125, 14)
// This is TIME-FIRST, matching the TFLite model's expected input.
float  window_flat[WINDOW_SIZE * NUM_CHANNELS];

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
const tflite::Model*      tfl_model   = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor*             tfl_input   = nullptr;
TfLiteTensor*             tfl_output  = nullptr;

// Tensor arena — tune this if AllocateTensors() fails (increase by 4KB steps)
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

  SENSE_SERIAL.begin(LINK_BAUD);

  // --- TFLite init ---
  tfl_model = tflite::GetModel(emotion_model);
  if (tfl_model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("ERROR: model schema version mismatch!");
    while (true) {}
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      tfl_model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("ERROR: AllocateTensors() failed — increase kTensorArenaSize");
    while (true) {}
  }

  tfl_input  = interpreter->input(0);
  tfl_output = interpreter->output(0);

  // Print tensor info for verification
  Serial.print("Input  shape : (1, ");
  Serial.print(tfl_input->dims->data[1]); Serial.print(", ");
  Serial.print(tfl_input->dims->data[2]); Serial.println(")");
  // Expected: (1, 125, 14)

  Serial.print("Input  dtype : "); Serial.println(tfl_input->type);
  // Expected: 9 = kTfLiteInt8

  Serial.print("Input  scale : "); Serial.println(tfl_input->params.scale, 6);
  Serial.print("Input  zp    : "); Serial.println(tfl_input->params.zero_point);

  Serial.print("Output scale : "); Serial.println(tfl_output->params.scale, 6);
  Serial.print("Output zp    : "); Serial.println(tfl_output->params.zero_point);

  Serial.println("Ready. Waiting for data from UNO...");
  Serial.println("FORMAT: millis,emotion,confidence,class_idx");
}

// ===========================================================================
void loop() {
  unsigned long now = millis();

  if (read_packet()) {
    if (now - last_sample_ms >= SAMPLE_INTERVAL_MS) {
      last_sample_ms = now;
      parse_and_buffer_packet();

      if (buffer_primed && samples_since_last_inference >= STEP_SIZE) {
        samples_since_last_inference = 0;
        run_inference();
      }
    }
  }
}

// ===========================================================================
// Read one framed packet from Serial1
// ===========================================================================
bool read_packet() {
  while (SENSE_SERIAL.available()) {
    uint8_t b = SENSE_SERIAL.read();
    if (b != START_BYTE) continue;

    pkt_buf[0] = START_BYTE;
    int bytes_read = 1;
    unsigned long deadline = millis() + 50;

    while (bytes_read < PACKET_TOTAL && millis() < deadline) {
      if (SENSE_SERIAL.available()) {
        pkt_buf[bytes_read++] = SENSE_SERIAL.read();
      }
    }

    if (bytes_read == PACKET_TOTAL) return true;

    Serial.print("WARN: incomplete packet (");
    Serial.print(bytes_read); Serial.println(" bytes)");
  }
  return false;
}

// ===========================================================================
// Parse packet → push one row [14] into circular buffer
// Buffer layout: sensor_buffer[write_idx][ch]  → time-major [N, 14]
// This matches Python: data_array shape is (N, 14)
// ===========================================================================
void parse_and_buffer_packet() {
  float imu1[IMU_FLOATS], imu2[IMU_FLOATS];
  memcpy(imu1, pkt_buf + 1,             IMU_BYTES);
  memcpy(imu2, pkt_buf + 1 + IMU_BYTES, IMU_BYTES);

  uint16_t emg1_raw, emg2_raw;
  memcpy(&emg1_raw, pkt_buf + 1 + IMU_BYTES * 2,             EMG_BYTES);
  memcpy(&emg2_raw, pkt_buf + 1 + IMU_BYTES * 2 + EMG_BYTES, EMG_BYTES);

  // Feature vector — 14 channels in training column order
  sensor_buffer[write_idx][0]  = imu1[IDX_AX];
  sensor_buffer[write_idx][1]  = imu1[IDX_AY];
  sensor_buffer[write_idx][2]  = imu1[IDX_AZ];
  sensor_buffer[write_idx][3]  = imu1[IDX_AVX];
  sensor_buffer[write_idx][4]  = imu1[IDX_AVY];
  sensor_buffer[write_idx][5]  = imu1[IDX_AVZ];
  sensor_buffer[write_idx][6]  = imu2[IDX_AX];
  sensor_buffer[write_idx][7]  = imu2[IDX_AY];
  sensor_buffer[write_idx][8]  = imu2[IDX_AZ];
  sensor_buffer[write_idx][9]  = imu2[IDX_AVX];
  sensor_buffer[write_idx][10] = imu2[IDX_AVY];
  sensor_buffer[write_idx][11] = imu2[IDX_AVZ];
  sensor_buffer[write_idx][12] = (float)emg1_raw;
  sensor_buffer[write_idx][13] = (float)emg2_raw;

  write_idx = (write_idx + 1) % BUFFER_SIZE;
  total_samples++;
  if (total_samples >= WINDOW_SIZE) buffer_primed = true;
  samples_since_last_inference++;
}

// ===========================================================================
// Extract window → z-score normalise → quantize → invoke → print result
//
// Replicates process_window() from Python exactly:
//   window = data_array[i:i + window_size]           → shape [125, 14]
//   w_mean = np.mean(window, axis=0, keepdims=True)  → per-channel mean
//   w_std  = np.std(window,  axis=0, keepdims=True)  → per-channel std
//   window_norm = (window - w_mean) / (w_std + 1e-8)
//
// TFLite model input: (1, 125, 14) — time-first.
// window_flat layout: [t * NUM_CHANNELS + ch]  → naturally time-first.
// ===========================================================================
void run_inference() {

  // ── 1. Extract latest WINDOW_SIZE rows from circular buffer ──────────────
  // start_idx points to the oldest sample in our window
  int start_idx = (write_idx - WINDOW_SIZE + BUFFER_SIZE) % BUFFER_SIZE;

  // Copy into window_flat as [t, ch] — time-first, matching (1, 125, 14)
  for (int t = 0; t < WINDOW_SIZE; t++) {
    int buf_idx = (start_idx + t) % BUFFER_SIZE;
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      window_flat[t * NUM_CHANNELS + ch] = sensor_buffer[buf_idx][ch];
    }
  }

  // ── 2. Per-channel z-score normalisation (matches Python axis=0) ─────────
  // Iterate over channels; for each channel gather all time steps.
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {

    // Mean across WINDOW_SIZE time steps for this channel
    float mean = 0.0f;
    for (int t = 0; t < WINDOW_SIZE; t++) {
      mean += window_flat[t * NUM_CHANNELS + ch];
    }
    mean /= (float)WINDOW_SIZE;

    // Population std (np.std uses ddof=0 by default)
    float var = 0.0f;
    for (int t = 0; t < WINDOW_SIZE; t++) {
      float diff = window_flat[t * NUM_CHANNELS + ch] - mean;
      var += diff * diff;
    }
    float std_dev = sqrtf(var / (float)WINDOW_SIZE);
    float denom   = std_dev + 1e-8f;

    // Normalise in-place
    for (int t = 0; t < WINDOW_SIZE; t++) {
      window_flat[t * NUM_CHANNELS + ch] = (window_flat[t * NUM_CHANNELS + ch] - mean) / denom;
    }
  }

  // ── 3. Quantize float → int8 and copy into TFLite input tensor ───────────
  // Formula: int8_val = round(float_val / scale) + zero_point
  float  in_scale = tfl_input->params.scale;
  int32_t in_zp   = tfl_input->params.zero_point;

  int8_t* input_data = tfl_input->data.int8;
  int     total_elements = WINDOW_SIZE * NUM_CHANNELS;

  for (int i = 0; i < total_elements; i++) {
    float quantized = (window_flat[i] / in_scale) + (float)in_zp;
    // Clamp to int8 range
    quantized = quantized < -128.0f ? -128.0f : (quantized > 127.0f ? 127.0f : quantized);
    input_data[i] = (int8_t)roundf(quantized);
  }

  // ── 4. Run inference ──────────────────────────────────────────────────────
  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("ERROR: Invoke() failed");
    return;
  }

  // ── 5. Dequantize output int8 → float probabilities ──────────────────────
  // Formula: float_val = (int8_val - zero_point) * scale
  float   out_scale = tfl_output->params.scale;
  int32_t out_zp    = tfl_output->params.zero_point;
  int8_t* out_data  = tfl_output->data.int8;

  float probs[NUM_EMOTIONS];
  for (int i = 0; i < NUM_EMOTIONS; i++) {
    probs[i] = ((float)out_data[i] - (float)out_zp) * out_scale;
  }

  // ── 6. Argmax ─────────────────────────────────────────────────────────────
  int   best_class = 0;
  float best_score = probs[0];
  for (int i = 1; i < NUM_EMOTIONS; i++) {
    if (probs[i] > best_score) {
      best_score = probs[i];
      best_class = i;
    }
  }

  // ── 7. Print result ───────────────────────────────────────────────────────
  // FORMAT: millis,emotion,confidence,class_idx
  Serial.print(millis());
  Serial.print(",");
  Serial.print(EMOTION_LABELS[best_class]);
  Serial.print(",");
  Serial.print(best_score, 4);
  Serial.print(",");
  Serial.println(best_class);
}
