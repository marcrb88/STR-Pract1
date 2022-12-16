#include "mbed.h"
#include <chrono>
#include <cstdint>
#include "lib/LightSensor.h"
#include "lib/Potentiometer.h"
#include "lib/Grove_LCD_RGB_Backlight.h"
#include <cstring>
#include <string> 
#include <iostream>

#define DEADLINE 500
#define QUEUE_SIZE 20
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

const float Rl = 10.0;
const float VREF = 3.3;
const float luxRel = 500.0;
const float ADCRES = 65535.0;
bool interrupt = false;
uint64_t mainStart, meanStart, lcdStart, mainRemain, now, interruptStart;
string lcd_message[2];

// CIRCULAR QUEUE FOR MEASURED DATA
int queue[QUEUE_SIZE];
int queue_index = 0;
bool queue_full = false;
bool should_calculate_mean = false;

// LCD OUTPUT
enum MESSAGE_TYPE { INFO, MEAN, MEASURING, ERR };
MESSAGE_TYPE actual_message = INFO;
int message_ticks = 0;
int message_colors[][3] = {
    {255, 255, 255},
    {0, 255, 0},
    {0, 0, 255},
    {255, 0, 0}
};

bool is_in_deadline() {
    return (Kernel::get_ms_count() - mainStart) <= DEADLINE;
}

int * get_color_by_message_type(MESSAGE_TYPE type) {
    switch (type) {
        case INFO: {
            return new int[3]{255, 255, 255};
        } case MEAN: {
            return new int[3]{0, 255, 0};
        } case MEASURING: {
            return new int[3]{0, 0, 255};
        } case ERR: {
            return new int[3]{255, 0, 0};
        }
    }
}

void set_LCD_message(string row1, string row2, MESSAGE_TYPE message_type)
{
    int * rgb = get_color_by_message_type(message_type);
    lcdStart = Kernel::get_ms_count();
    char output [row1.length() + 1];
    char output2 [row2.length() + 1];
    strcpy(output, row1.c_str());
    strcpy(output2, row2.c_str());
    actual_message = message_type;

    lcd.setRGB(rgb[0], rgb[1], rgb[2]);
    lcd.clear();
    lcd.print(output);
    lcd.locate(0, 1);
    lcd.print(output2);
}

/**
 * Shows error message and activates the alarm
 */
void alert(string message = "") {
    buzzer.write(0.25);

    message_ticks = 6;
    actual_message = ERR;
    set_LCD_message(message, "", ERR);
}

void calculate_Mean() {
    meanStart = Kernel::get_ms_count();
    mean = 0;

    for (int i = 0; i < QUEUE_SIZE; i++) {
        mean += queue[i];
    }
    mean = mean / QUEUE_SIZE;
    printf("Temps calcul mitjana: %llu\n", Kernel::get_ms_count() - meanStart);

    string message = "Mitjana lux:";
    string message2 = std::to_string(mean) + "%";
    set_LCD_message(message, message2, MEAN);
    message_ticks = 6;
    button.enable_irq();
}

/**
 * Disables button IRQ enables mean calculation flag.
 */
void RSI_button () {
    button.disable_irq();
    should_calculate_mean = true;
}

int main() {
    uint16_t counts;
    float vout, mean, compensation, max_compensation;

    buzzer.period(0.10);

    button.rise(&RSI_button);

    while (true) {
        mainStart = Kernel::get_ms_count();

        // DESHABILITEM EL BUZZER
        buzzer.write(0);

        // DETECTAR LUMINOSITAT
        counts = lightSensor.read();
        vout = lightSensor.calculate_Vout(counts);

        if (vout < 0) alert("LIGHT SENSOR ERR");
   
        lux = lightSensor.calculate_percentage(vout);

        // REGISTRE MESURES
        queue[queue_index] = lux;
        queue_index = (queue_index + 1) % QUEUE_SIZE;
        if (!queue_full && queue_index == 0) queue_full = true;

        // COMPENSAR LLUM
        compensation = 100 - lux;
        max_compensation = potentiometer.read() * 100;

        if (max_compensation < 0) alert("MAX COMP ERR");

        if (compensation > max_compensation) compensation = max_compensation;

        led.write(compensation/100); 

        printf("Temps calcul main: %llu\n", Kernel::get_ms_count() - mainStart);

        if (is_in_deadline()) {
            // ACTUALITZAR LCD
            if (message_ticks <= 0) {
                string message = "Lux: " + std::to_string(lux) + "%";
                string message2 = "Comp: " + std::to_string(compensation) + "%";
                
                set_LCD_message(message, message2, INFO);
            } else {
                message_ticks--;
            }

            if (should_calculate_mean) {
                if (queue_full) {
                    calculate_Mean();
                    should_calculate_mean = false;
                } else {
                    string message = "Collecting data";
                    string message2 = "...";
                    set_LCD_message(message, message2, MEASURING);
                    message_ticks = QUEUE_SIZE - queue_index;
                }
            }

            mainRemain = DEADLINE - (Kernel::get_ms_count() - mainStart);
            printf("Temps restant: %llu \n", mainRemain);
            printf("Consumint temps restant\n\n");
            ThisThread::sleep_for(mainRemain);
        }
        else alert("OUT OF DEADLINE!");
    }
}
