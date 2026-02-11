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
  delay(2000);
  pinMode(LED_BUILTIN, OUTPUT); // Initialise built in LED for debugging
  Serial.println("t_ms,ax1,ay1,az1,gx1,gy1,gz1,yaw1,pitch1,roll1,ax2,ay2,az2,gx2,gy2,gz2,yaw2,pitch2,roll2,emg1_data,emg2_data");//printing header


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
      
      // Write IMU 1 data
      Serial.print(imu1_data.ax); Serial.print(",");
      Serial.print(imu1_data.ay); Serial.print(",");
      Serial.print(imu1_data.az); Serial.print(",");
      Serial.print(imu1_data.r); Serial.print(",");
      Serial.print(imu1_data.p); Serial.print(",");
      Serial.print(imu1_data.y); Serial.print(",");
      Serial.print(imu1_data.avx); Serial.print(",");
      Serial.print(imu1_data.avy); Serial.print(",");
      Serial.print(imu1_data.avz);Serial.print(",");

      // Write IMU 2 data
      Serial.print(imu2_data.ax); Serial.print(",");
      Serial.print(imu2_data.ay); Serial.print(",");
      Serial.print(imu2_data.az); Serial.print(",");
      Serial.print(imu2_data.r); Serial.print(",");
      Serial.print(imu2_data.p); Serial.print(",");
      Serial.print(imu2_data.y); Serial.print(",");
      Serial.print(imu2_data.avx); Serial.print(",");
      Serial.print(imu2_data.avy); Serial.print(",");
      Serial.print(imu2_data.avz);Serial.print(",");

      // Write EMG 1 data
      Serial.print(emg1_data);Serial.print(",");

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

}
