#include <Wire.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>


// mention EMG output port

// Sampling
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
  int emg = analogRead(EMG_PIN);

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

  Serial.println(emg);
}
