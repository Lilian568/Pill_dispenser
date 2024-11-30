#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

#define SW_0 9  // Replace with the actual GPIO pin number for SW_0
#define SW_2 7  // Replace with the actual GPIO pin number for SW_2

// Initialize button GPIO pins
void buttonsInit();

// Check if a button is pressed
bool isButtonPressed(int button_pin);

#endif // BUTTON_H