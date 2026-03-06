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
  Serial1.begin(38400); // For UART from Uno
  delay(2000);
  pinMode(LED_BUILTIN, OUTPUT); // Initialise built in LED for debugging
  Serial.println("t_ms,ax1,ay1,az1,gx1,gy1,gz1,yaw1,pitch1,roll1,ax2,ay2,az2,gx2,gy2,gz2,yaw2,pitch2,roll2,emg1,emg2");//printing header


}

void loop() {
  // put your main code here, to run repeatedly:
  // Print to serial montior the values received
  if (Serial1.available()) {
    if(Serial1.read() == 0xAA) {
      imuData imu1_data;
      imuData imu2_data;
      uint16_t emg1_data = 0;
      uint16_t emg2_data = 0;

      // Read IMU and EMG data
      Serial1.readBytes((uint8_t*)&imu1_data, sizeof(imu1_data));
      Serial1.readBytes((uint8_t*)&imu2_data, sizeof(imu2_data));
      Serial1.readBytes((uint8_t*)&emg1_data, sizeof(emg1_data));
      Serial1.readBytes((uint8_t*)&emg2_data, sizeof(emg2_data));

      // Write millis
      Serial.print(millis()); Serial.print(",");
      
      // Write IMU 1 Data
      Serial.print(data.ax); Serial.print(",");
      Serial.print(data.ay); Serial.print(",");
      Serial.print(data.az); Serial.print(",");
      Serial.print(data.r); Serial.print(",");
      Serial.print(data.p); Serial.print(",");
      Serial.print(data.y); Serial.print(",");
      Serial.print(data.avx); Serial.print(",");
      Serial.print(data.avy); Serial.print(",");
      Serial.print(data.avz);Serial.print(",");

      // Write IMU 2 data
      Serial.print(data.ax); Serial.print(",");
      Serial.print(data.ay); Serial.print(",");
      Serial.print(data.az); Serial.print(",");
      Serial.print(data.r); Serial.print(",");
      Serial.print(data.p); Serial.print(",");
      Serial.print(data.y); Serial.print(",");
      Serial.print(data.avx); Serial.print(",");
      Serial.print(data.avy); Serial.print(",");
      Serial.print(data.avz);Serial.print(",");

      // Write EMG 1 data
      Serial.print(emg1);Serial.print(",");

      // Write EMG 2 data
      Serial.println(emg2_data);


    }
//    int data_in = Serial1.parseInt();
//
//    Serial.print("Val received: ");
//    Serial.println(data_in);
//
//    if (data_in == 8) { digitalWrite(LED_BUILTIN, HIGH); }
//    else { digitalWrite(LED_BUILTIN, LOW); }
  }
17707,0.04,-0.08,-0.05,-5.37,47.31,319.31,0.00,-0.13,0.06,0.04,-0.08,-0.05,-5.37,47.31,319.31,0.00,-0.13,0.06,55050,48291

}
