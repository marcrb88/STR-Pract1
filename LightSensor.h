#include "mbed.h"
#include <cstdint>

class LightSensor {
public:
  LightSensor(AnalogIn pin, PwmOut buzzer);
  uint16_t read();
  float calculate_Vout(uint16_t counts);
  float calculate_Lux(float vout);
  float calculate_percentage(float vout);

private:
  AnalogIn _lightsensor;
  PwmOut _buzzer;
  uint16_t _counts;
};
