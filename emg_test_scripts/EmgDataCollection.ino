const int emgPin = A3;
const int buttonPin = 2;

// --- CONFIGURATION ---
const int numReadings = 10;   // Higher = Smoother line, but slightly slower lag
int readings[numReadings];    // The array to store the readings
int readIndex = 0;            // The index of the current reading
long total = 0;               // The running total
int average = 0;              // The final smoothed value

// Button Variables
bool isRecording = false;
bool lastButtonState = HIGH;
unsigned long debounceTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);

  // Initialize the smoothing array to 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
}

void loop() {
  // --- 1. CHECK BUTTON ---
  int buttonReading = digitalRead(buttonPin);
  if (buttonReading == LOW && lastButtonState == HIGH && millis() - debounceTime > 200) {
    isRecording = !isRecording;
    debounceTime = millis();
    if(isRecording) Serial.println("--- START ---");
    else Serial.println("--- STOP ---");
  }
  lastButtonState = buttonReading;

  // --- 2. READ & PROCESS EMG ---
  if (isRecording) {
    
    // Step A: Read raw value
    int rawValue = analogRead(emgPin);
    
    // Step B: RECTIFY (Center the signal and make it positive)
    // Most EMG sensors sit at ~512 (2.5V) when neutral. 
    // We subtract 512 to get "0" center, then take absolute value.
    // NOTE: Adjust "512" if your sensor's resting value is different!
    int rectifiedValue = abs(rawValue - 210); 

    // Step C: RUNNING AVERAGE (Smoothing)
    total = total - readings[readIndex];       // Subtract the oldest reading
    readings[readIndex] = rectifiedValue;      // Add the new reading
    total = total + readings[readIndex];       // Add the new reading to the total
    readIndex = readIndex + 1;                 // Advance to the next position

    if (readIndex >= numReadings) {            // Wrap around to the beginning
      readIndex = 0;
    }

    average = total / numReadings;             // Calculate the average

    // Step D: PRINT
    // We print "0" and "1023" to stop the Serial Plotter from auto-scaling constantly
    Serial.print(0);      // Lower bound
    Serial.print(" ");
    Serial.print(300);    // Upper bound (adjust if your flex goes higher)
    Serial.print(" ");
    Serial.println(average); // The clean signal
    // Serial.println(rawValue); // The clean signal

    
    delay(10); // Small delay to keep readable (100Hz approx)
  }
}