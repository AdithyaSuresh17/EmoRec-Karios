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
  // put your setup code here, to run once:
  // Serial initialisations
  Serial.begin(115200); // For USB serial monitor
  Serial1.begin(9600); // For UART from Uno

  pinMode(LED_BUILTIN, OUTPUT); // Initialise built in LED for debugging

}

void loop() {
  // put your main code here, to run repeatedly:
  // Print to serial montior the values received
  if (Serial1.available()) {
    if(Serial1.read() == 0xAA) {
      imuData data;
      Serial1.readBytes((uint8_t*)&data, sizeof(data));

      Serial.print(millis()); Serial.print(",");
      Serial.print(data.ax); Serial.print(",");
      Serial.print(data.ay); Serial.print(",");
      Serial.print(data.az); Serial.print(",");
      Serial.print(data.r); Serial.print(",");
      Serial.print(data.p); Serial.print(",");
      Serial.print(data.y); Serial.print(",");
      Serial.print(data.avx); Serial.print(",");
      Serial.print(data.avy); Serial.print(",");
      Serial.println(data.avz);

    }
//    int data_in = Serial1.parseInt();
//
//    Serial.print("Val received: ");
//    Serial.println(data_in);
//
//    if (data_in == 8) { digitalWrite(LED_BUILTIN, HIGH); }
//    else { digitalWrite(LED_BUILTIN, LOW); }
  }

}
