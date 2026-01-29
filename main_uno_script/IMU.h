#include <Arduino.h>

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

class IMU {
  public:
    IMU(Adafruit_BNO055*); // Constructor
    void init();
    imuData getSensorData();
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
