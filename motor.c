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

int steps_per_revolution = -1; // Steps required for one full revolution
int steps_per_drop = -1;       // Calibrated steps per one pill dispense
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

// Rotate the motor by a specified number of steps
void rotate(int step_count) {
    static int current_position = 0;
    for (int i = 0; i < step_count; i++) {
        current_position = (current_position + 1) % 8;
        setSteps(current_position);
    }
}
// Fine-tune the motor's position to ensure proper alignment
void fine_tune_position() {
    printf("Fine tuning the position\n");
    rotate(steps_per_revolution / 12); // Perform a small rotation for fine alignment
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

// Calibrate the motor to determine steps per revolution and steps per pill dispense
void calibrate() {
    printf("Starting calibration...\n");

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
            printf("Valid revolution %d: %d steps\n", i, step_count);
        } else {
            printf("Ignored revolution %d: %d steps\n", i + 1, step_count);
        }

        sleep_ms(100);
    }

    // Calculate steps per revolution and steps per pill dispense
    steps_per_revolution = revolutions_total / valid_iterations;
    steps_per_drop = steps_per_revolution / 8; // One pill = 1/8 revolution
    is_calibrated = true;

    printf("Calibration complete. Average steps per revolution: %d\n", steps_per_revolution);
    printf("Steps per drop: %d\n", steps_per_drop);

    // Align the motor to the start position
    while (gpio_get(OPTOFORK_PIN)) {
        rotate(1);
    }
    while (!gpio_get(OPTOFORK_PIN)) {
        rotate(1);
    }
    fine_tune_position();

    printf("Motor aligned to start position.\n");
}

void rotate_steps_512() {
    if (!is_calibrated) { // Ensure motor is calibrated before rotation
        printf("Motor is not calibrated. Cannot rotate.\n");
        return;
    }
    printf("Rotating %d steps (1 drop)...\n", steps_per_drop);
    rotate(steps_per_drop); // Rotate the motor by the calibrated steps for one pill
}