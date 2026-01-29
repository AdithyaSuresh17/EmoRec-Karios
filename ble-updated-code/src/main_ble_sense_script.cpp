#include <Arduino.h>
#include <Wire.h>
// Struct definition
struct imuData {
  float ax;
  float ay;
  float az;
  float r;
  float p;
  float y;
  float avx;
  float avy;
  float avz;
};


void setup() {
  // Serial initialisations
  Serial.begin(115200);   // USB serial monitor
  Serial1.begin(9600);    // UART from Uno

  pinMode(LED_BUILTIN, OUTPUT); // Built in LED for debugging
}

void loop() {
  if (Serial1.available()) {

    // Look for start byte
    if (Serial1.read() == 0xAA) {

      imuData data;

      // Read binary struct
      size_t bytesRead = Serial1.readBytes(
        reinterpret_cast<uint8_t*>(&data),
        sizeof(data)
      );

      // Ensure full struct was received
      if (bytesRead == sizeof(data)) {

        Serial.print(millis()); Serial.print(",");
        Serial.print(data.ax);  Serial.print(",");
        Serial.print(data.ay);  Serial.print(",");
        Serial.print(data.az);  Serial.print(",");
        Serial.print(data.r);   Serial.print(",");
        Serial.print(data.p);   Serial.print(",");
        Serial.print(data.y);   Serial.print(",");
        Serial.print(data.avx); Serial.print(",");
        Serial.print(data.avy); Serial.print(",");
        Serial.println(data.avz);
      }
    }
  }
}
