#include "led.h"
#include "button.h"
#include "motor.h"
#include "piezo.h"
#include <stdio.h>
#include "pico/stdlib.h"

// Debug printing macro
#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

int main() {
    // Initializing
    timer_hw->dbgpause = 0;
    stdio_init_all();
    ledsInit();
    pwmInit();
    buttonsInit();
    setup();
    setupPiezoSensor();

    int state = 1;
    absolute_time_t last_time_sw2_pressed = nil_time; // Timestamp for dispensing
    const uint32_t step_delay_ms = 15000; // Delay between motor rotations in milliseconds
    int portion_count = 0; // Counter for dispensed portions
    const int max_portion = 7; // Maximum allowed portions before resetting

    bool state1_logged = false;
    bool state2_logged = false;

    while (true) {
        absolute_time_t current_time = get_absolute_time(); // Get the current time

        switch (state) {
            case 1: // Calibration state
                if (!state1_logged) {
                    printf("Press SW0, to start the calibration.\n");
                    state1_logged = true;
                }

                blink();

                if (isButtonPressed(SW_0)) {
                    calibrate();
                    state = 2;
                    state1_logged = false;
                    state2_logged = false;
                    last_time_sw2_pressed = nil_time;
                }
                break;

            case 2: // Dispensing state
                if (!state2_logged) {
                    printf("Press SW2, to activate dispensing.\n");
                    allLedsOn();
                    state2_logged = true;
                }

                // Time calculations for motor rotating
                uint64_t time_since_last_press = is_nil_time(last_time_sw2_pressed)
                                                     ? 0
                                                     : absolute_time_diff_us(last_time_sw2_pressed, current_time) / 1000;

                if (isButtonPressed(SW_2)) {
                    allLedsOff();
                    printf("Pill dispensing activated.\n");

                    if (!dispensingAndDetecting()) {
                        DEBUG_PRINT("Pill not detected! Blinking LED 5 times.");
                        blinkE(5);
                    } else {
                        DEBUG_PRINT("Pill successfully dispensed.");
                    }
                    portion_count ++;
                    last_time_sw2_pressed = current_time;
                    DEBUG_PRINT("Portion count incremented to %d", portion_count);

                } else if (time_since_last_press >= step_delay_ms && !is_nil_time(last_time_sw2_pressed)) {
                    DEBUG_PRINT("Attempting to dispense a pill");

                    if (!dispensingAndDetecting()) {
                        DEBUG_PRINT("Pill not detected! Blinking LED 5 times.");
                        blinkE(5);
                    } else {
                        DEBUG_PRINT("Pill successfully dispensed.");
                    }
                    portion_count++;
                    last_time_sw2_pressed = current_time;
                    DEBUG_PRINT("Portion count incremented to %d", portion_count);
                }

                if (portion_count >= max_portion) { // Check if max portions have been dispensed
                    DEBUG_PRINT("Max portion count reached.");
                    portion_count = 0;
                    allLedsOff();
                    state = 1;
                    state1_logged = false;
                    state2_logged = false;
                }
                break;

            default: // Handle unexpected states
                DEBUG_PRINT("Unknown state encountered: %d", state);
                break;
        }

        sleep_ms(200);
    }

    return 0;
}
