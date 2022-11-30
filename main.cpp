#include "mbed.h"
#include <chrono>
#include <cstdint>
#include "lib/LightSensor.h"
#include "lib/Potentiometer.h"
#include "lib/Grove_LCD_RGB_Backlight.h"
#include <cstring>
#include <string> 
#include <iostream>

using namespace std::chrono;

// INPUTS
LightSensor lightSensor(A0);
InterruptIn button(D4);
Potentiometer potentiometer(A3);

// OUTPUTS
PwmOut led(D3);
PwmOut buzzer(D5);
Grove_LCD_RGB_Backlight lcd(D14,D15);

Timer t;
float lux = 0, mean = 0;
bool buttonPressed = false;

const float Rl = 10.0;
const float VREF = 3.3;
const float luxRel = 500.0;
const float ADCRES = 65535.0;

float calculate_Mean() {
    float add = 0;
    int tics = 0;
    
    t.start();
    while (duration_cast<chrono::seconds>(t.elapsed_time()).count() <= 10.0){
        tics++;
        add += lux;
    }
    t.stop();
    t.reset();

    return add/tics;
}

void RSI_button () {
   button.disable_irq();
   buttonPressed = true;
}

void setLCDMessage(string row1, string row2, int rgb[3])
{
    char output [row1.length() + 1];
    char output2 [row2.length() + 1];
    strcpy(output, row1.c_str());
    strcpy(output2, row2.c_str());

    lcd.setRGB(rgb[0], rgb[1], rgb[2]);
    lcd.clear();
    lcd.print(output);
    lcd.locate(0, 1);
    lcd.print(output2);
}

// main() runs in its own thread in the OS
int main() {
    uint16_t counts;
    float vout, mean;

    //periods
    buzzer.period(0.10);

    button.rise(&RSI_button);
    
    while (true) {
        if (buttonPressed) {

            mean = calculate_Mean();
            string message = "Mitjana lux:";
            string message2 = std::to_string(mean) + "%";

            setLCDMessage(message, message2, (int[3]) {255, 0, 0});
            ThisThread::sleep_for(3s);

            button.enable_irq();
            buttonPressed = false;
        }

        counts = lightSensor.read();
        vout = lightSensor.calculate_Vout(counts);
   
        lux = lightSensor.calculate_percentage(vout);
        if (vout < 0)
            buzzer.write(0.25);
        
        float compensation = 100 - lux;
        float max_compensation = potentiometer.read() * 100;

        if (compensation > max_compensation) compensation = max_compensation;

        led.write(compensation/100); 

        string message = "Lux: " + std::to_string(lux) + "%";
        string message2 = "Comp: " + std::to_string(compensation) + "%";
 

        ThisThread::sleep_for(200ms);
        setLCDMessage(message, message2, (int[3]) {255, 255, 255});
      
        buzzer.write(0.0);
    }

}
