#include "EMG.h"

EMG::EMG(int pin) {
  _pin = pin;
  _baseline = 512; // Default
  _readIndex = 0;
  _total = 0;
  for (int i = 0; i < _numReadings; i++) _readings[i] = 0;
}

void EMG::calibrate() {
  long sum = 0;
  int samples = 200;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(_pin);
    delay(5); // Quick sampling for baseline
  }
  _baseline = sum / samples;
}

int EMG::getProcessedData() {
  int rawValue = analogRead(_pin);
  
  // 1. Rectify using the calibrated baseline
  int rectified = abs(rawValue - _baseline);

  // 2. Smooth (Moving Average)
  _total = _total - _readings[_readIndex];
  _readings[_readIndex] = rectified;
  _total = _total + _readings[_readIndex];
  _readIndex = (_readIndex + 1) % _numReadings;

  return _total / _numReadings;
}

int EMG::getBaseline() {
  return _baseline;
}