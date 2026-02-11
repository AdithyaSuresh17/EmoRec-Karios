#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "IMU.h"
#include "EMG.h" 
#include <SoftwareSerial.h>

// --- OBJECT CREATION ---
Adafruit_BNO055 bno1 = Adafruit_BNO055(55, 0x28);
Adafruit_BNO055 bno2 = Adafruit_BNO055(56, 0x29);
IMU imu1(&bno1); 
IMU imu2(&bno2); 

// Create two EMG objects on pins A0 and A3
EMG emg1(A0); 
EMG emg2(A3);

// --- COMMUNICATIONS & CONTROL ---
SoftwareSerial sense_link(8, 9);
const int buttonPin = 2; // Pin for calibration toggle

void setup() {
  Serial.begin(115200);
  sense_link.begin(9600);
  
  //pinMode(buttonPin, INPUT_PULLUP);

  // Initialise IMUs
  imu1.init();
  imu2.init();

  // --- AUTO-CALIBRATION PHASE ---
  /*Serial.println(">>> HOLD STILL: Press button to calibrate EMGs...");
  while(digitalRead(buttonPin) == HIGH); // Wait for user to press button
  
  Serial.println(">>> Calibrating Baseline...");
  emg1.calibrate(); // Performs the 200-sample average
  emg2.calibrate();
  
  Serial.println(">>> Calibration Complete. Starting Transmission.");
  delay(1000);*/
}

void loop() {
  // 1. Gather IMU Data
  imuData imu_data_pkt1;
  imuData imu_data_pkt2;
  
  imu_data_pkt1 = imu1.getSensorData();
  delay(50); // Reduced delay for better responsiveness
  imu_data_pkt2 = imu2.getSensorData();
  delay(50);

  // 2. Gather Processed EMG Data
  uint16_t processedEMG1 = (uint16_t) emg1.getProcessedData(); 
  uint16_t processedEMG2 = (uint16_t) emg2.getProcessedData();

  // 3. Send Data to BLE Sense
  const uint8_t start_byte = 0xAA;
  sense_link.write(start_byte);
  
  // Sending IMU 1 packet
  sense_link.write((uint8_t*)&imu_data_pkt1, sizeof(imu_data_pkt1));
  // Sending IMU 2 packet
  sense_link.write((uint8_t*)&imu_data_pkt2, sizeof(imu_data_pkt2));
  


  sense_link.write((uint8_t*)&processedEMG1, sizeof(processedEMG1));
  sense_link.write((uint8_t*)&processedEMG2, sizeof(processedEMG2));

  // --- DEBUG MONITORING ---
  /*Serial.print("EMG1: "); Serial.print(processedEMG1);
  Serial.print(" | EMG2: "); Serial.print(processedEMG2);
  Serial.print(" | Roll1: "); Serial.print(imu_data_pkt1.r);
  Serial.print(" | Roll2: "); Serial.println(imu_data_pkt2.r);*/

}