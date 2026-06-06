#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "IMU.h"

// Constructor
IMU::IMU(Adafruit_BNO055* bno_ptr) {
  this->bno = bno_ptr;
}

void IMU::init() {
  if(!this->bno->begin()) {
    Serial.print("No BNO055 detected, check wiring or I2C ADDR");
    while(1); // Do not allow script to proceed
  }
  } else {
    Serial.println("BNO055 detected");
  }
  delay(500);
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


// Added Calibration support for IMU 
bool IMU::isCalibrated() {
    uint8_t sys, gyro, accel, mag;
    // getCalibration pulls the current status from the BNO055 (0 = uncalibrated, 3 = fully calibrated)
    bno->getCalibration(&sys, &gyro, &accel, &mag);
    
    // We want all sensors to be fully calibrated
    return (sys == 3 && gyro == 3 && accel == 3 && mag == 3);
}

void IMU::printCalibrationStatus() {
    uint8_t sys, gyro, accel, mag;
    bno->getCalibration(&sys, &gyro, &accel, &mag);
    
    Serial.print("System: "); Serial.print(sys);
    Serial.print(" | Gyro: "); Serial.print(gyro);
    Serial.print(" | Accel: "); Serial.print(accel);
    Serial.print(" | Mag: "); Serial.println(mag);
}


void IMU::calibrate() {
    Serial.println("--- IMU Calibration Started ---");
    Serial.println("Gyro: Leave still.");
    Serial.println("Accel: Rest on 6 different sides.");
    Serial.println("Mag: Move in a figure-8 pattern.");
    
    // This loop blocks everything until the IMU returns true for isCalibrated()
    while (!isCalibrated()) {
        printCalibrationStatus();
        delay(500); // Check twice a second so we don't spam the Serial Monitor
    }
    Serial.println("--- IMU Fully Calibrated! ---");
}