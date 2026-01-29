#include <Wire.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <utility/imumaths.h>


Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

// Sampling delay
const unsigned long sampling_periodMs = 500;
unsigned long lastSampleMs = 0;

void printHeader() {
  Serial.println("t_ms,ax,ay,az,gx,gy,gz,yaw,pitch,roll");
}

void setup() {
  Serial.begin(115200);
  delay(200);

  bno.begin();
  delay(100);

  printHeader();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleMs < sampling_periodMs) return;
  lastSampleMs = now;

  /* -------- Orientation (yaw, pitch, roll) -------- */
  sensors_event_t event;
  bno.getEvent(&event);

  float yaw   = event.orientation.x;
  float pitch = event.orientation.y;
  float roll  = event.orientation.z;

  /* -------- Accelerometer + Gyroscope -------- */
  imu::Vector<3> acc_data  = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
  imu::Vector<3> gyro_data = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);

  float ax = acc_data.x();
  float ay = acc_data.y();
  float az = acc_data.z();

  float gx = gyro_data.x();
  float gy = gyro_data.y();
  float gz = gyro_data.z();

  /* -------- CSV output -------- */
  Serial.print(now); Serial.print(",");

  Serial.print(ax, 6); Serial.print(",");
  Serial.print(ay, 6); Serial.print(",");
  Serial.print(az, 6); Serial.print(",");

  Serial.print(gx, 6); Serial.print(",");
  Serial.print(gy, 6); Serial.print(",");
  Serial.print(gz, 6); Serial.print(",");

  Serial.print(yaw, 6);   Serial.print(",");
  Serial.print(pitch, 6); Serial.print(",");
  Serial.println(roll, 6);
}

// mention EMG output port

// Sampling
/*
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);
const unsigned long sampling_periodMs = 20; 
unsigned long lastSampleMs = 0;

void printHeader() {
  Serial.println("t_ms,ax,ay,az,gx,gy,gz,roll,pitch,yaw,emg_adc");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  bno.begin();

  delay(100);   // small settle time
  printHeader();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleMs < sampling_periodMs) return;
  lastSampleMs = now;

  // IMU vectors
  sensors_event_t accelEvent, gyroEvent, orientEvent;

  bno.getEvent(&accelEvent, Adafruit_BNO055::VECTOR_ACCELEROMETER);
  bno.getEvent(&gyroEvent,  Adafruit_BNO055::VECTOR_GYROSCOPE);
  bno.getEvent(&orientEvent,Adafruit_BNO055::VECTOR_EULER);

  float ax = accelEvent.acceleration.x;
  float ay = accelEvent.acceleration.y;
  float az = accelEvent.acceleration.z;

  float gx = gyroEvent.gyro.x;
  float gy = gyroEvent.gyro.y;
  float gz = gyroEvent.gyro.z;

  float roll  = orientEvent.orientation.roll;
  float pitch = orientEvent.orientation.pitch;
  float yaw   = orientEvent.orientation.heading;


  // EMG raw
  //int emg = analogRead(EMG_PIN);

  // Printing data on serial for python file to read
  Serial.print(now); Serial.print(",");

  Serial.print(ax, 6); Serial.print(",");
  Serial.print(ay, 6); Serial.print(",");
  Serial.print(az, 6); Serial.print(",");

  Serial.print(gx, 6); Serial.print(",");
  Serial.print(gy, 6); Serial.print(",");
  Serial.print(gz, 6); Serial.print(",");

  Serial.print(roll, 6);  Serial.print(",");
  Serial.print(pitch, 6); Serial.print(",");
  Serial.print(yaw, 6);   Serial.print(",");

  //Serial.println(emg);
}
*/