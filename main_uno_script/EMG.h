#ifndef EMG_H
#define EMG_H

#include <Arduino.h>

class EMG {
  public:
    EMG(int pin);           // Constructor (needs the analog pin)
    void calibrate();       // Finds the dynamic baseline
    int getProcessedData(); // Does the Rectification and Smoothing
    int getBaseline();      // Returns the calculated baseline

  private:
    int _pin;
    int _baseline;
    
    // Smoothing variables
    static const int _numReadings = 10;
    int _readings[_numReadings];
    int _readIndex;
    long _total;
};

#endif