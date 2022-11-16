#include "mbed.h"
#include <chrono>
#include <cstdint>
#include "LightSensor.h"
#include "Grove_LCD_RGB_Backlight.h"
#include <cstring>
#include <string> 
#include <iostream>

using namespace std::chrono;

PwmOut buzzer(D5);
LightSensor lightSensor(A0, buzzer);
PwmOut led(D3);
InterruptIn button(D4);
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

Grove_LCD_RGB_Backlight lcd(D14,D15);
// main() runs in its own thread in the OS
int main() {
    uint16_t counts;
    float vout, mean;

    //periods
    buzzer.period(0.01);

    button.rise(&RSI_button);
    
    while (true) {

        if (buttonPressed) {

            mean = calculate_Mean();
            string message = "Mitjana lux:";
            string message2 = std::to_string(mean) + "%";
            
            char output [message.length() + 1];
            char output2 [message2.length() + 1];

            strcpy(output, message.c_str());
            strcpy(output2, message2.c_str());

            lcd.setRGB(255, 0, 0);
            lcd.clear();
            lcd.print(output);
            lcd.locate(0,1);
            lcd.print(output2);
            ThisThread::sleep_for(3s);

            button.enable_irq();
            buttonPressed = false;
        }

        counts = lightSensor.read();
        vout = lightSensor.calculate_Vout(counts);
        lux = lightSensor.calculate_percentage(vout);
        float compensation = 100 - lux;

        led.write(compensation/100); 

        string message = "Lux: " + std::to_string(lux) + "%";
        string message2 = "Comp: " + std::to_string(compensation) + "%";
 
        char output [message.length() + 1];
        char output2 [message2.length() + 1];
        strcpy(output, message.c_str());
        strcpy(output2, message2.c_str());
        ThisThread::sleep_for(200ms);
        lcd.setRGB(255, 255, 255);
        lcd.clear();
        lcd.print(output);
        lcd.locate(0,1);
        lcd.print(output2);

        buzzer.write(0.0);
    }

}
