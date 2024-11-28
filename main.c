#include "led.h"
#include "button.h"
#include "motor.h"
#include <stdio.h>
#include "pico/stdlib.h"


bool isButtonPressed(int button_pin) {
    return gpio_get(button_pin) == 0;
}
int main() {
    stdio_init_all();
    ledsInit();
    pwmInit();
    buttonsInit();
    setup();
    int state = 1;
    uint32_t last_time_sw2_pressed = 0;
    const uint32_t step_delay_ms = 30000;
    int portion_count = 0;
    const int max_portion = 8;
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
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
                    if (current_time - last_time_sw2_pressed >= step_delay_ms) {
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

