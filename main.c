#include "led.h"
#include "button.h"
#include <stdio.h>
#include "pico/stdlib.h"


bool isButtonPressed() {
    return gpio_get(SW_0) == 0;
}
int main() {
    stdio_init_all();
    ledsInit();
    pwmInit();
    buttonsInit();

    while (true) {
        blink();

        if (isButtonPressed()) {
            allLedsOff();
            break;
        }
    }

    return 0;
}
//yes okay
