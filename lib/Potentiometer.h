#include "mbed.h"
#include "AnalogIn.h"

class Potentiometer
{
    public:
        Potentiometer(AnalogIn pin);
        float read();
    
    private:
        AnalogIn _potentiometer;
};
