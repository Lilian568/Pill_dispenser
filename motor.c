#include <stdio.h>
#include <unistd.h>
#include "pico/stdlib.h"
#include <string.h>
#include "motor.h"

#define OPTOFORK_PIN 28
#define MOTOR_PIN1 2
#define MOTOR_PIN2 3
#define MOTOR_PIN3 6
#define MOTOR_PIN4 13
#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

int steps_per_revolution = -1; // Steps for full revolution
int steps_per_drop = -1;       // Steps between one block (per pill dispense)
bool is_calibrated = false;

// Motor step sequence for an 8-step motor
void setSteps(int step) {
    bool steps[8][4] = {
        {1, 0, 0, 0},
        {1, 1, 0, 0},
        {0, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 1, 1},
        {0, 0, 0, 1},
        {1, 0, 0, 1}
    };
    gpio_put(MOTOR_PIN1, steps[step][0]);
    gpio_put(MOTOR_PIN2, steps[step][1]);
    gpio_put(MOTOR_PIN3, steps[step][2]);
    gpio_put(MOTOR_PIN4, steps[step][3]);
    sleep_ms(1);
}

// Rotate the motor
void rotate(int step_count) {
    static int current_position = 0;
    for (int i = 0; i < step_count; i++) {
        current_position = (current_position + 1) % 8;
        setSteps(current_position);
    }
}
// Fine-tuning the motor position to align the piezo sensor just below the hole
void fine_tune_position() {
    rotate(steps_per_revolution / 12);
}

void setup() {
    gpio_init(OPTOFORK_PIN);
    gpio_set_dir(OPTOFORK_PIN, GPIO_IN);
    gpio_pull_up(OPTOFORK_PIN);

    gpio_init(MOTOR_PIN1);
    gpio_set_dir(MOTOR_PIN1, GPIO_OUT);
    gpio_init(MOTOR_PIN2);
    gpio_set_dir(MOTOR_PIN2, GPIO_OUT);
    gpio_init(MOTOR_PIN3);
    gpio_set_dir(MOTOR_PIN3, GPIO_OUT);
    gpio_init(MOTOR_PIN4);
    gpio_set_dir(MOTOR_PIN4, GPIO_OUT);
}

// Calibrating steps per revolution and steps per dispense
void calibrate() {
    printf("Calibrating motor...\n");

    int step_count = 0;
    int revolutions_total = 0;
    const int valid_iterations = 1;
    const int actual_iterations = valid_iterations + 1;

    for (int i = 0; i < actual_iterations; i++) {
        step_count = 0;

        // Rotate until the optofork detects the start position
        while (gpio_get(OPTOFORK_PIN)) {
            rotate(1);
            step_count++;
        }
        sleep_ms(100);

        // Rotate until the optofork no longer detects the start position
        while (!gpio_get(OPTOFORK_PIN)) {
            rotate(1);
            step_count++;
        }

        // Continue rotating until the optofork detects the start position again
        while (gpio_get(OPTOFORK_PIN)) {
            rotate(1);
            step_count++;
        }

        if (i > 0) { // Ignore the first revolution for accuracy
            revolutions_total += step_count;
        }
        sleep_ms(100);
    }

    // Calculate steps per revolution and steps per pill dispense
    steps_per_revolution = revolutions_total / valid_iterations;
    steps_per_drop = steps_per_revolution / 8; // One pill = 1/8 revolution
    is_calibrated = true;


    // Align the motor to the start position
    while (gpio_get(OPTOFORK_PIN)) {
        rotate(1);
    }
    while (!gpio_get(OPTOFORK_PIN)) {
        rotate(1);
    }
    fine_tune_position();
    DEBUG_PRINT("Steps per revolution: %d. Steps per drop: %d\n", steps_per_revolution, steps_per_drop);
    printf("Calibration complete.\n");
}

void rotate_steps_512() {
    if (!is_calibrated) { // Ensure motor is calibrated before rotation
        DEBUG_PRINT("Motor is not calibrated. Cannot rotate.\n");
        return;
    }
    rotate(steps_per_drop); // Rotate the motor by the calibrated steps for one pill
}
