#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "IMU.h"
#include <SoftwareSerial.h> // For sending data from Uno to BLE Sense

// Create objects
Adafruit_BNO055 bno1 = Adafruit_BNO055(55, 0x28);
Adafruit_BNO055 bno2 = Adafruit_BNO055(56, 0x29);
IMU imu1(&bno1); // Pass in address of bno1 object
IMU imu2(&bno2); // Pass in address of bno1 object

// Initialise software serial
// Uno TX (D9) --> Sense RX0
// Uno RX (D8) --> Sense TX1
SoftwareSerial sense_link(8,9);

void setup() {
  // put your setup code here, to run once:
  // Serial initialisations
  Serial.begin(115200); // For USB serial monitor
  sense_link.begin(9600); // Baud for UART to BLE Sense
  

  // Initialise IMUs
  imu1.init();
  imu2.init();
  
  

}

void loop() {
  // put your main code here, to run repeatedly:
  // TEST - Print IMU sensor data
//  Serial.println("Print");
//  delay(100);
//  while(1) {
//    Serial.print("imu1: ");
//    imu1.getSensorData();
//    delay(100);
//    Serial.print(" ||| imu2: ");
//    imu2.getSensorData();
//    delay(100);
//    Serial.println(" ");
//  }

  // TEST - Sending data to another Arduino
//  sense_link.println(8); // Send value as ASCII + newline
//  delay(100);
//  sense_link.println(88); // Send value as ASCII + newline
//  delay(100);

  // Send IMU values to BLE Sense
  imu1.getSensorData();
  delay(100);
  imu2.getSensorData();
  delay(100);
  int data_to_send = imu1.getRoll();
  Serial.println(data_to_send);
  sense_link.println(data_to_send);
  delay(100);

}
