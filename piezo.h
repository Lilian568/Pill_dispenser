#ifndef PIEZO_H
#define PIEZO_H

#include <stdbool.h>
#include "led.h"

#define PIEZO_SENSOR_PIN 27 // GPIO pin for the Piezo sensor

// Initialize the Piezo sensor
void setupPiezoSensor();

// Check if a pill was dispensed
bool isPillDispensed();

// Blink an error using LEDs
void blinkError(int times);

#endif // PIEZO_H