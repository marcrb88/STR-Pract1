#include "mbed.h"
#include "Potentiometer.h"

const float MAX_VALUE = 1.0;

Potentiometer::Potentiometer(AnalogIn pin) : _potentiometer(pin)
{
    _potentiometer = pin;
}

float Potentiometer::read()
{
    float value = _potentiometer.read();

    if (value < 0) value = 0;
    if (value > MAX_VALUE) value = MAX_VALUE;

    return value / MAX_VALUE;
}
