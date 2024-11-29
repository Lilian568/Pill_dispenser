//
// Created by lily on 11/25/2024.
//
#include "button.h"
#include "hardware/gpio.h"

//GLOBAL VARIABLES
static const int button_arr[] = {SW_0, SW_2};

//BUTTON FUNCTIONS
void buttonsInit() {
    gpio_init(SW_0);
    gpio_set_dir(SW_0, GPIO_IN);
    gpio_pull_up(SW_0); // Jos käytät pull-up vastuksia.

    gpio_init(SW_2);
    gpio_set_dir(SW_2, GPIO_IN);
    gpio_pull_up(SW_2); // Sama SW_2:lle.
}