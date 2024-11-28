#include "led.h"
#include "button.h"
#include "motor.h"
#include <stdio.h>
#include "pico/stdlib.h"

bool isButtonPressed(int button_pin) {
    static bool last_state = false;
    bool current_state = gpio_get(button_pin);
    if (current_state != last_state) {
        last_state = current_state;
        if (current_state) {
            return true;
        }
    }
    return false;
}

int main() {
    stdio_init_all();
    ledsInit();
    pwmInit();
    buttonsInit();
    setup();

    int state = 1;
    absolute_time_t last_time_sw2_pressed = get_absolute_time();
    const uint32_t step_delay_ms = 30000;
    int portion_count = 0;
    const int max_portion = 7;

    while (true) {
        absolute_time_t current_time = get_absolute_time();

        switch (state) {
            case 1:
                blink();
                if (isButtonPressed(SW_0)) {
                    calibrate();
                    state = 2;
                }
                break;

            case 2:
                if (isButtonPressed(SW_2)) {
                    allLedsOff();

                    if (absolute_time_diff_us(last_time_sw2_pressed, current_time) >= step_delay_ms * 1000) {
                        rotate_steps_512();
                        portion_count++;
                        last_time_sw2_pressed = current_time;
                    }

                    if (portion_count >= max_portion) {
                        portion_count = 0;
                        allLedsOff();
                    }
                }
                break;

            default:
                break;
        }

        sleep_ms(100);
    }

    return 0;
}

