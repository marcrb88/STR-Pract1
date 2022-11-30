#include "mbed.h"
#include <cstdint>

class LightSensor {
public:
  LightSensor(AnalogIn pin);
  uint16_t read();
  float calculate_Vout(uint16_t counts);
  float calculate_Lux(float vout);
  float calculate_percentage(float vout);

private:
  AnalogIn _lightsensor;
  uint16_t _counts;
};
