#include "mbed.h"
#include <chrono>
#include <cstdint>
#include "lib/LightSensor.h"
#include "lib/Potentiometer.h"
#include "lib/Grove_LCD_RGB_Backlight.h"
#include <cstring>
#include <string> 
#include <iostream>

#define DEADLINE 200

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
bool buttonPressed = false, display = false;

const float Rl = 10.0;
const float VREF = 3.3;
const float luxRel = 500.0;
const float ADCRES = 65535.0;
uint16_t start;
string msgMean, msgvalueMean, msgLux, msgComp;
Thread thread;
uint64_t now;


float calculate_Mean() {
    float add = 0;
    int tics = 0;

    while ((Kernel::get_ms_count() - now) < 10000){
        tics++;
        add += lux;
    }
  
    return add/tics;
}

bool is_in_deadline() {
    
    return (Kernel::get_ms_count() - start) > DEADLINE ?  false : true;
}

void RSI_button () {
   button.disable_irq();
   buttonPressed = true;
}

void setLCDMessage()
{
    while (true) {
        if (buttonPressed && display) {
            buttonPressed = false;
            display = false;

            char output [msgMean.length() + 1];
            char output2 [msgvalueMean.length() + 1];
            strcpy(output, msgMean.c_str());
            strcpy(output2, msgvalueMean.c_str());
        
            lcd.setRGB(255, 0, 0);
            lcd.clear();
            lcd.print(output);
            lcd.locate(0, 1);
            lcd.print(output2);
            ThisThread::sleep_for(3s);

        } else {
            char output [msgLux.length() + 1];
            char output2 [msgComp.length() + 1];
            strcpy(output, msgLux.c_str());
            strcpy(output2, msgComp.c_str());

            lcd.setRGB(255, 255, 255);
            lcd.clear();
            lcd.print(output);
            lcd.locate(0, 1);
            lcd.print(output2);
            ThisThread::sleep_for(300ms);
        }
    }
    
}

// main() runs in its own thread in the OS
int main() {
    uint16_t counts;
    float vout, mean, compensation, max_compensation;

    //periods
    buzzer.period(0.10);

    button.rise(&RSI_button);

    while (true) {
        start = Kernel::get_ms_count();
        thread.start(setLCDMessage);

        if (buttonPressed) { 
            now = Kernel::get_ms_count();
            mean = calculate_Mean();
            printf("\nmitjana:%f \n", mean);
            display = true;

            msgMean = "Mitjana lux:";
            msgvalueMean = std::to_string(mean) + "%";
            thread.start(setLCDMessage);
            ThisThread::sleep_for(2s); //escencial pel sincronisme entre main i thread, sino la variable buttonPressed mai es posara a false i tornara a fer mitjana.
            button.enable_irq();
        }
        
        counts = lightSensor.read();
        vout = lightSensor.calculate_Vout(counts);

        if (vout < 0) buzzer.write(0.25);
   
        lux = lightSensor.calculate_percentage(vout);

        compensation = 100 - lux;
        max_compensation = potentiometer.read() * 100;

        if (is_in_deadline()) {
            if (max_compensation < 0) buzzer.write(0.25);

            if (compensation > max_compensation) compensation = max_compensation;

            led.write(compensation/100); 

            msgLux = "Lux: " + std::to_string(lux) + "%";
            msgComp = "Comp: " + std::to_string(compensation) + "%";
 
            ThisThread::sleep_for(500ms);
       
        }
        // else buzzer.write(0.25);
        
    }

}
