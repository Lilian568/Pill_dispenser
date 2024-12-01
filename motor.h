#ifndef MOTOR_H
#define MOTOR_H

#include <stdbool.h>

// External variable representing the number of steps per revolution
extern int steps_per_revolution;

// Flag indicating whether the motor has been calibrated
extern bool is_calibrated;

// Initialize the motor and associated hardware
void setup();

// Set the motor steps to a specific pattern
void setSteps(int step);

// Rotate the motor by a given number of steps
void rotate(int step_count);

// Fine-tune the motor's position for precise alignment
void fine_tune_position();

// Calibrate the motor to determine steps per revolution
void calibrate();

// Rotate the motor by the calibrated number of steps for one pill dispense
void rotate_steps_512();


#endif // MOTOR_H