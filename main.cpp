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
bool error_handled = false;

const float Rl = 10.0;
const float VREF = 3.3;
const float luxRel = 500.0;
const float ADCRES = 65535.0;
int interrupts;
uint64_t mainStart, meanStart, lcdStart, mainRemain, now, interruptStart;
string lcd_message[2];


float calculate_Mean() {
    meanStart = Kernel::get_ms_count();
    float add = 0;
    int tics = 0;

    while ((Kernel::get_ms_count() - now) < 10000){
        tics++;
        add += lux;
    }
    
    printf("Temps calcul mitjana: %llu\n", Kernel::get_ms_count() - meanStart);
    return add/tics;
}

bool is_in_deadline() {
    return (Kernel::get_ms_count() - mainStart) <= DEADLINE;
}

void RSI_button () {
    if (Kernel::get_ms_count() - mainStart > 35) {
        button.disable_irq();
        buttonPressed = true;
    }
    interrupts ++;
}

void set_LCD_message(string row1, string row2, int rgb[3])
{
    lcdStart = Kernel::get_ms_count();
    char output [row1.length() + 1];
    char output2 [row2.length() + 1];
    strcpy(output, row1.c_str());
    strcpy(output2, row2.c_str());

    lcd.setRGB(rgb[0], rgb[1], rgb[2]);
    lcd.clear();
    lcd.print(output);
    lcd.locate(0, 1);
    lcd.print(output2);

    printf("Temps calcul print: %llu\n", Kernel::get_ms_count() - lcdStart);
}

void set_LCD_alertMessage(string row0, string row1 = "") {
    lcd_message[0] = row0;
    lcd_message[1] = row1;
}

void alert(string message = "") {
    error_handled = true;

    buzzer.write(0.25);

    set_LCD_alertMessage(message);

    ThisThread::sleep_for(ERROR_INFO_TIME);
    buzzer.write(0);
    error_handled = false;
}


int main() {
    uint16_t counts;
    float vout, mean, compensation, max_compensation;

    buzzer.period(0.10);

    button.rise(&RSI_button);

    while (true) {
        mainStart = Kernel::get_ms_count();

        while (interrupts > 0 || buttonPressed) {
            interruptStart = Kernel::get_ms_count();
            now = Kernel::get_ms_count();
            mean = calculate_Mean();
            string message = "Mitjana lux:";
            string message2 = std::to_string(mean) + "%";
            set_LCD_message(message, message2, (int[3]) {0, 255, 0});

            interrupts --;
            buttonPressed = false;
            button.enable_irq();
        }
        
        counts = lightSensor.read();
        vout = lightSensor.calculate_Vout(counts);

        if (vout < 0) alert("LIGHT SENSOR ERR");
   
        lux = lightSensor.calculate_percentage(vout);

        compensation = 100 - lux;
        max_compensation = potentiometer.read() * 100;

        if (is_in_deadline()) {
            if (max_compensation < 0) alert("MAX COMP ERR");

            if (compensation > max_compensation) compensation = max_compensation;

            led.write(compensation/100); 

            string message = "Lux: " + std::to_string(lux) + "%";
            string message2 = "Comp: " + std::to_string(compensation) + "%";
            
            set_LCD_message(message, message2, (int[3]) {255, 255, 255});

            printf("Temps calcul main: %llu\n", Kernel::get_ms_count() - mainStart);

            mainRemain = DEADLINE - (Kernel::get_ms_count() - mainStart);
            printf ("Temps restant: %llu \n", mainRemain);
            if (is_in_deadline()) 
                ThisThread::sleep_for(mainRemain);
        }
        else alert("OUT OF DEADLINE!");
    }
}
