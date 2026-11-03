#pragma once
#include <stdint.h>
struct DisplayCalibration {
  int16_t red=100,green=100,blue=100,gamma=100,saturation=100,contrast=100;
};
bool validCalibration(const DisplayCalibration&);
bool identityCalibration(const DisplayCalibration&);
void buildCalibrationLut(const DisplayCalibration&,uint16_t*lut);
