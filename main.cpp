#include "mbed.h"
#include <chrono>
#include <cstdint>
#include "lib/LightSensor.h"
#include "lib/Potentiometer.h"
#include "lib/Grove_LCD_RGB_Backlight.h"
#include <cstring>
#include <string> 
#include <iostream>

#define DEADLINE 600
#define ERROR_INFO_TIME 2s

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
bool display = false;
bool error_handled = false;

const float Rl = 10.0;
const float VREF = 3.3;
const float luxRel = 500.0;
const float ADCRES = 65535.0;
uint16_t start;
string lcd_message[2];
Thread thread;
uint64_t now;

Mutex lcd_mutex;

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
    return (Kernel::get_ms_count() - start) <= DEADLINE;
}

void RSI_button () {
   button.disable_irq();
   buttonPressed = true;
}

void lcdBackgroundJob()
{
    chrono::milliseconds sleep_time = 300ms;
    int rgb[3] = {255, 255, 255};

    while (true) {
        printf("\nHOLA2 \n");
        lcd_mutex.lock();
        char output [lcd_message[0].length() + 1];
        char output2 [lcd_message[1].length() + 1];
        strcpy(output, lcd_message[0].c_str());
        strcpy(output2, lcd_message[1].c_str());
        lcd_mutex.unlock();
        
        if (buttonPressed && display) {
            buttonPressed = false;
            display = false;

            rgb[1] = 255;
            rgb[2] = 0;
            sleep_time = 3s;
        } else if (error_handled) {
            rgb[1] = 0;
            rgb[2] = 0;
            sleep_time = ERROR_INFO_TIME;
        } else {
            rgb[1] = 255;
            rgb[2] = 255;
            sleep_time = 300ms;
        }

        lcd.setRGB(rgb[0], rgb[1], rgb[2]);
        lcd.clear();
        lcd.print(output);
        lcd.locate(0, 1);
        lcd.print(output2);
        ThisThread::sleep_for(sleep_time);
    }   
}

void setLCDMessage(string row0, string row1 = "") {
    lcd_mutex.lock();
    lcd_message[0] = row0;
    lcd_message[1] = row1;
    lcd_mutex.unlock();
}

void alert(bool error, string message = "") {
    error_handled = error;

    if (error) {
        buzzer.write(0.25);

        setLCDMessage(message);

        ThisThread::sleep_for(ERROR_INFO_TIME);
        buzzer.write(0);
        error_handled = false;
    }
}


int main() {
    uint16_t counts;
    float vout, mean, compensation, max_compensation;

    buzzer.period(0.10);

    button.rise(&RSI_button);

    thread.start(lcdBackgroundJob);
    printf("\nHOLA \n");

    while (true) {
        alert(false);
        start = Kernel::get_ms_count();

        if (buttonPressed) { 
            now = Kernel::get_ms_count();
            setLCDMessage("Collecting data",
                          "...");

            mean = calculate_Mean();
            printf("\nmitjana:%f \n", mean);
            display = true;

            setLCDMessage("Mitjana lux:",
                          std::to_string(mean) + "%");
            ThisThread::sleep_for(2s); //escencial pel sincronisme entre main i thread, sino la variable buttonPressed mai es posara a false i tornara a fer mitjana.
            button.enable_irq();
        }
        
        counts = lightSensor.read();
        vout = lightSensor.calculate_Vout(counts);

        if (vout < 0) alert(true, "LIGHT SENSOR ERR");
   
        lux = lightSensor.calculate_percentage(vout);
        printf("\nlux:%f \n", lux);

        compensation = 100 - lux;
        max_compensation = potentiometer.read() * 100;

        if (is_in_deadline()) {
            if (max_compensation < 0) alert(true, "MAX COMP ERR");

            if (compensation > max_compensation) compensation = max_compensation;

            led.write(compensation/100); 

            setLCDMessage("Lux:  " + std::to_string(lux) + "%",
                          "Comp: " + std::to_string(compensation) + "%");
 
            ThisThread::sleep_for(500ms);
        }
        else alert(true, "OUT OF DEADLINE!");
    }
}
