//
// Created by tomit on 28/11/2024.
//

#ifndef MOTOR_H
#define MOTOR_H
#include <stdbool.h>
extern int steps_per_revolution;
extern bool is_calibrated;

void setup();
void setSteps(int step);
void rotate(int step_count);
void calibrate();
void rotate_steps_512();
int do_command(char *command);

#endif //MOTOR_H
