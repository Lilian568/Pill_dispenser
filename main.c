#include "led.h"
#include "button.h"
#include "motor.h"
#include "piezo.h"
#include <stdio.h>
#include "pico/stdlib.h"

// Macro for debug printing
#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

int main() {
    // Initialize components
    timer_hw->dbgpause = 0;
    stdio_init_all();
    ledsInit();
    pwmInit();
    buttonsInit();
    setup();
    setupPiezoSensor();

    int state = 1;
    absolute_time_t last_time_sw2_pressed = nil_time; // Timestamp of the last SW_2 press
    const uint32_t step_delay_ms = 30000; // Delay between motor rotations in milliseconds
    int portion_count = 0; // Counter for dispensed portions
    const int max_portion = 7; // Maximum allowed portions before resetting

    bool state1_logged = false;
    bool state2_logged = false;

    while (true) {
        absolute_time_t current_time = get_absolute_time(); // Get the current time

        switch (state) {
            case 1: // Calibration state
                if (!state1_logged) {
                    DEBUG_PRINT("State 1: Waiting for SW_0 button to start calibration.");
                    state1_logged = true;
                }

                blink(); // Blink LEDs to indicate waiting in calibration state

                if (isButtonPressed(SW_0)) {
                    DEBUG_PRINT("SW_0 button pressed, starting calibration...");
                    calibrate();
                    state = 2;
                    state1_logged = false;
                    state2_logged = false;
                    DEBUG_PRINT("Calibration complete, moving to state 2.");
                    last_time_sw2_pressed = nil_time;
                }
                break;

            case 2: // Dispensing state
                if (!state2_logged) {
                    DEBUG_PRINT("State 2: Monitoring SW_2 for motor activation.");
                    allLedsOn();
                    state2_logged = true;
                }

                // Calculate time since last SW_2 button press
                uint64_t time_since_last_press = is_nil_time(last_time_sw2_pressed)
                                                     ? 0
                                                     : absolute_time_diff_us(last_time_sw2_pressed, current_time) / 1000;

                if (isButtonPressed(SW_2)) {
                    allLedsOff();
                    DEBUG_PRINT("SW_2 button pressed. Immediate motor activation...");
                    rotate_steps_512();
                    if (!isPillDispensed()) {
                        DEBUG_PRINT("Pill not detected! Blinking LED 5 times.");
                        blinkError(5);
                    }
                    portion_count++;
                    last_time_sw2_pressed = current_time;
                    DEBUG_PRINT("Portion count incremented to %d after SW_2 press.", portion_count);

                } else if (time_since_last_press >= step_delay_ms && !is_nil_time(last_time_sw2_pressed)) {
                    // Check if delay period has passed since last motor activation
                    DEBUG_PRINT("Step delay met, rotating motor...");
                    rotate_steps_512();
                    if (!isPillDispensed()) {
                        DEBUG_PRINT("Pill not detected! Blinking LED 5 times.");
                        blinkError(5);
                    }
                    portion_count++;
                    last_time_sw2_pressed = current_time;
                    DEBUG_PRINT("Portion count incremented to %d", portion_count);
                }

                if (portion_count >= max_portion) { // Check if max portions have been dispensed
                    DEBUG_PRINT("Max portion count reached. Resetting portion count and turning off LEDs.");
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