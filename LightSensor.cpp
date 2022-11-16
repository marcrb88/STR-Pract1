#include "LightSensor.h"
#include "mbed.h"
#include <cstdint>

const float MAX_INTENSITY = 500.0;
const float ADCRES = 65535.0;
const float VREF = 3.3;
const float lUXREL = 500.0;
const float R1 = 10.0;

uint16_t _counts;

LightSensor::LightSensor(AnalogIn lightsensor, PwmOut buzzer) : _lightsensor(lightsensor), _buzzer(buzzer) {
 _lightsensor = lightsensor;
 _buzzer = buzzer;
}

uint16_t LightSensor::read() {
    _counts = _lightsensor.read_u16();

    return _counts;
}

float LightSensor::calculate_Vout(uint16_t counts) {
    return (counts * VREF) / ADCRES;
}

float LightSensor::calculate_Lux(float vout) {
    float lux = ((((VREF*lUXREL)*vout) - lUXREL) / R1);
    if (lux < 70 ) {
        lux = 0;
        _buzzer.write(0.5);
    }

    return lux;
}

float LightSensor::calculate_percentage(float vout) {
    return calculate_Lux(vout)* 100 / MAX_INTENSITY;


}