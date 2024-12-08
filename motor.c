#include "pico/stdlib.h"
#include "steppermotor.h"
#include <stdio.h>
#include "eeprom.h"

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DEBUG_PRINT(f_, ...)
#endif

static const int stepper_array[] = {IN1, IN2, IN3, IN4};
static const uint turning_sequence[8][4] = {{1, 0, 0, 0},
                                            {1, 1, 0, 0},
                                            {0, 1, 0, 0},
                                            {0, 1, 1, 0},
                                            {0, 0, 1, 0},
                                            {0, 0, 1, 1},
                                            {0, 0, 0, 1},
                                            {1, 0, 0, 1}};

static volatile int row = 0;

volatile int calibration_count;
volatile int revolution_counter = 0;
volatile int calibration_count = 0;
volatile bool calibrated = false;
volatile bool fallingEdge = false;
volatile bool pill_detected = false;



void stepperMotorInit() {//Initializes stepper motor.
    for (int i = 0; i < sizeof(stepper_array) / sizeof(stepper_array[0]); i++) {
        gpio_init(stepper_array[i]);
        gpio_set_dir(stepper_array[i], GPIO_OUT);
    }
}

void calibrateMotor() {//  Calibrates motor by rotating the stepper motor and counting the number opf steps between two falling edge
    calibrated = false;
    fallingEdge = false;
    while (false == fallingEdge) {
        runMotorClockwise(1);
    }
    fallingEdge = false;
    while (false == fallingEdge) {
        runMotorClockwise(1);
    }
    calibrated = true;
    DBG_PRINT("Number of steps per revolution: %u\n", calibration_count);
    runMotorAntiClockwise(ALIGNMENT);
}


void runMotorAntiClockwise(int times) {//Rotates stepper motor anticlockwise by the number of integer passed as parameter.

    for(int i  = 0; i < times; i++) {
        for (int j = 0; j < sizeof(stepper_array) / sizeof(stepper_array[0]); j++) {
            gpio_put(stepper_array[j], turning_sequence[row][j]);
        }
        if (++row >= 8) {
            row = 0;
        }
        sleep_ms(2);
    }
}


    for(; times > 0; times--) {//Rotates stepper motor clockwise by the number of integer passed as parameter.
        for (int j = 0; j < sizeof(stepper_array) / sizeof(stepper_array[0]); j++) {
            gpio_put(stepper_array[j], turning_sequence[row][j]);
        }
        revolution_counter++;
        if (--row <= -1) {
            row = 7;
        }
        sleep_ms(2);
    }
}

void realignMotor() {//If reboot occurs during motor turn, realigns motor back to last stored position.
    int stored_position = i2cReadByte(STEPPER_POSITION_ADDRESS) * 4;
    while (0 != stored_position--) {
        runMotorAntiClockwise(1);
    }
}

void optoforkInit() {
    gpio_init(OPTOFORK);
    gpio_set_dir(OPTOFORK, GPIO_IN);
    gpio_pull_up(OPTOFORK);
}

void optoFallingEdge() { //In case of optofork falling edge, fallingEdge flag is set to true. Sets the calibration_count and resets the revolution_counter to zero.
    fallingEdge = true;
    if (false == calibrated) {
        calibration_count = revolution_counter;
    }
    revolution_counter = 0;
}

void piezoInit() {
    gpio_init(PIEZO);
    gpio_set_dir(PIEZO, GPIO_IN);
    gpio_pull_up(PIEZO);
}

void piezoFallingEdge() {//In case of piezo sensor falling edge, pill_detected flag is set to true.
    pill_detected = true;
}

void gpioFallingEdge(uint gpio, uint32_t event_mask) {
    if (OPTOFORK == gpio) {
        optoFallingEdge();
    } else {
        piezoFallingEdge();
    }
}
