#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "IMU.h"

// Create objects
Adafruit_BNO055 bno1 = Adafruit_BNO055(55, 0x28);
Adafruit_BNO055 bno2 = Adafruit_BNO055(56, 0x29);
IMU imu1(&bno1); // Pass in address of bno1 object
IMU imu2(&bno2); // Pass in address of bno1 object

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // Initialise serial
  Serial.println("Started");

  // Initialise IMUs
  imu1.init();
  imu2.init();
  
  

}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Print");
  delay(100);
  while(1) {
    Serial.print("imu1: ");
    imu1.getSensorData();
    delay(100);
    Serial.print(" ||| imu2: ");
    imu2.getSensorData();
    delay(100);
    Serial.println(" ");
  }

}
