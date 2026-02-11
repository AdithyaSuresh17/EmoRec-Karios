const int emgPin = A3;
const int buttonPin = 2;

// --- DSP & SMOOTHING VARIABLES ---
const int numReadings = 10;
int readings[numReadings];
int readIndex = 0;
long total = 0;
int smoothedValue = 0;

// --- CALIBRATION VARIABLES ---
int dynamicBaseline = 512; // Default starting point
const int calSamples = 200; // Number of samples to average for baseline
int calCount = 0;
long calSum = 0;

// --- STATE MACHINE ---
enum State { IDLE, CALIBRATING, RECORDING };
State currentState = IDLE;

bool lastButtonState = HIGH;
unsigned long debounceTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  for (int i = 0; i < numReadings; i++) readings[i] = 0;
}

void loop() {
  // 1. BUTTON LOGIC (Transition between States)
  int buttonReading = digitalRead(buttonPin);
  if (buttonReading == LOW && lastButtonState == HIGH && millis() - debounceTime > 200) {
    debounceTime = millis();
    
    if (currentState == IDLE) {
      currentState = CALIBRATING;
      calCount = 0;
      calSum = 0;
      Serial.println("--- PHASE 1: CALIBRATING (STAY STILL) ---");
    } else {
      currentState = IDLE;
      Serial.println("--- STOPPED ---");
    }
  }
  lastButtonState = buttonReading;

  // 2. EXECUTE STATES
  if (currentState == CALIBRATING) {
    int raw = analogRead(emgPin);
    calSum += raw;
    calCount++;

    if (calCount >= calSamples) {
      dynamicBaseline = calSum / calSamples;
      currentState = RECORDING;
      Serial.print("--- BASELINE FOUND: ");
      Serial.print(dynamicBaseline);
      Serial.println(" | STARTING RECORDING ---");
    }
    delay(10); // 100Hz sampling
  } 
  
  else if (currentState == RECORDING) {
    int rawValue = analogRead(emgPin);
    
    // RECTIFICATION using the Dynamic Baseline
    int rectifiedValue = abs(rawValue - dynamicBaseline);

    // SMOOTHING (Moving Average)
    total = total - readings[readIndex];
    readings[readIndex] = rectifiedValue;
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % numReadings;
    smoothedValue = total / numReadings;

    // Output for Serial Plotter
    Serial.print(0);             // Min line
    Serial.print(",");
    Serial.print(dynamicBaseline - 20);           // Max line
    Serial.print(",");
    Serial.println(smoothedValue);
    
    delay(10);
  }
}