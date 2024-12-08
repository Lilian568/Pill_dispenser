#ifndef PIEZO_H
#define PIEZO_H

#include <stdbool.h>
#include "led.h"

#define PIEZO_SENSOR_PIN 27 // GPIO pin for the Piezo sensor

// Initialize the Piezo sensor
void setupPiezoSensor(void);

// Enable Piezo sensor interrupt
void enablePiezoInterrupt(void);

// Disable Piezo sensor interrupt
void disablePiezoInterrupt(void);

// Dispense a pill and detect it using the piezo sensor
bool dispensingAndDetecting(void);

// Blink an error using LEDs
void blinkError(int times);

#endif // PIEZO_H
