#ifndef MOTOR_H
#define MOTOR_H

#include <stdbool.h>

// External variable representing the number of steps per revolution
extern int steps_per_revolution;

// External variable representing the number of steps for one pill dispense
extern int steps_per_drop;

// Flag indicating whether the motor has been calibrated
extern bool is_calibrated;

// Initialize the motor and associated hardware
void setup();

// Set the motor steps to a specific pattern
void setSteps(int step);

// Rotate the motor by a given number of steps with an option to update EEPROM
void rotate(int step_count, bool update_eeprom);

// Fine-tune the motor's position for precise alignment
void fine_tune_position();

// Calibrate the motor to determine steps per revolution
void calibrate();

// Rotate the motor by the calibrated number of steps for one pill dispense
void rotate_steps_512();

// Test EEPROM connection
void test_eeprom_connection();

// Check and realign motor position if necessary
void realign_after_interruption();

void reset_and_realign();


// motor.h
void maybe_update_eeprom_periodically(void);

// Getter for the current motor step
int get_current_motor_step();

// Setter for the current motor step
void set_current_motor_step(int step);

#endif // MOTOR_H
