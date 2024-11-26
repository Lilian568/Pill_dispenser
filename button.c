//
// Created by lily on 11/25/2024.
//
#include "button.h"
#include "hardware/gpio.h"

//GLOBAL VARIABLES
static const int button_arr[] = {SW_0, SW_2};

//BUTTON FUNCTIONS
void buttonsInit() {
    for (int i = 0; i < sizeof(button_arr)/sizeof(button_arr[0]); i++) {
        gpio_init(button_arr[i]);
        gpio_set_dir(button_arr[i], GPIO_IN);
        gpio_pull_up(button_arr[i]);
    }
}