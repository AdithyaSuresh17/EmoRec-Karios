#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "IMU.h"

// Constructor
IMU::IMU(Adafruit_BNO055* bno_ptr) {
  this->bno = bno_ptr;
}

// Sensor initialisation
void IMU::init() {
  
  // Sensor initialisation
  if(!this->bno->begin()) { // If there is a problem detecting BNO055
    Serial.print("No BNO055 detected, check wiring or I2C ADDR");
    while(1); // Do not allow script to proceed
  }
  else {
    Serial.println("BNO055 detected");
  }
  
  delay(500);

  // Configure BNO055 clock source to external crystal (more precise) instead of internal oscillator
  this->bno->setExtCrystalUse(true);
}

// Read IMU sensor data, and return
imuData IMU::getSensorData() {
  /* Get sensor angle data */
  // Read data
  sensors_event_t event;
  this->bno->getEvent(&event);

  // Store yaw, pitch, roll
  this->yaw = event.orientation.x;
  this->pitch = event.orientation.y;
  this->roll = event.orientation.z;

  /* Get accelerometer and gyroscope sensor data */
  // Read data
  imu::Vector<3> acc_data = this->bno->getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
  imu::Vector<3> gyro_data = this->bno->getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
  
  // Store acceleration (accelerometer) data
  this->x_acc = acc_data.x();
  this->y_acc = acc_data.y();
  this->z_acc = acc_data.z();
  
  // Store angular velocity (gyro) data
  this->x_angVel = gyro_data.x();
  this->y_angVel = gyro_data.y();
  this->z_angVel = gyro_data.z();

  /* DEBUGGING */
//  Serial.print("Yaw: ");
//  Serial.print(this->yaw);
//  Serial.print(" || Pitch: ");
//  Serial.print(this->pitch);
//  Serial.print(" || Roll: ");
//  Serial.print(this->roll);
//
//  Serial.print("x acc: ");
//  Serial.print(this->x_acc);
//  Serial.print(" || y acc: ");
//  Serial.print(this->y_acc);
//  Serial.print(" || z_acc: ");
//  Serial.print(this->z_acc);
//
//  Serial.print("x ang vel: ");
//  Serial.print(this->x_angVel);
//  Serial.print(" || y ang vel: ");
//  Serial.print(this->y_angVel);
//  Serial.print(" || z_angVel: ");
//  Serial.print(this->z_angVel);

  // Place values into an output struct
  imuData data;
  data.ax = this->x_acc;
  data.ay = this->y_acc;
  data.az = this->z_acc;
  data.r = this->roll;
  data.p = this->pitch;
  data.y = this->yaw;
  data.avx = this->x_angVel;
  data.avy = this->y_angVel;
  data.avz = this->z_angVel;
  
  return data;
}

int IMU::getRoll() {
  return this->roll;
}
