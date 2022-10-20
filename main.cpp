#include "mbed.h"
#include <cstdint>
#include "LightSensor.h"


LightSensor lightSensor(A0);

const float Rl = 10.0;
const float VREF = 3.3;
const float luxRel = 500.0;
const float ADCRES = 65535.0;

/*float calculateVoid(uint16_t counts) {
    return (counts * VREF) / 65535;
}*/

float calculateLightIntensity (float vout) {
    return ((((VREF * luxRel) * vout) - luxRel) / Rl);
}

// main() runs in its own thread in the OS
int main() {
    uint16_t counts;
    float vout;

    while (true) {
        counts = lightSensor.read();
        vout = lightSensor.calculate_Vout(counts);
        printf("Counts %d Intensitat de llum: %f \n", counts, lightSensor.calculate_Lux(vout)); 
        ThisThread::sleep_for(1000ms);
    }
}

/*
while (true) {
    brightness = get_brightness();

    led.setIntensity(100-brightness);

    manageErrors();

    if (tick % FREQUENCY == 0) {
        print("Brightness of room: $brightness, LED intensity: 100-brightness");
    } 
}

*/