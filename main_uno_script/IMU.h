#include <Arduino.h>

class IMU {
  public:
    IMU(Adafruit_BNO055*); // Constructor
    void init();
    void getSensorData();
    int getRoll();

  private:
    Adafruit_BNO055* bno; // Pointer to BNO object this class instance is made for
    float x_acc;
    float y_acc;
    float z_acc;
    float roll;
    float pitch;
    float yaw;
    float x_angVel;
    float y_angVel;
    float z_angVel;
  
};
