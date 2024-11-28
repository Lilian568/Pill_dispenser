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
#define STEPS_PER_REVOLUTION 4096

int steps_per_revolution = -1;
bool is_calibrated = false;

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

void rotate(int step_count) {
    static int current_position = 0;
    for (int i = 0; i < step_count; i++) {
        current_position = (current_position + 1) % 8;
        setSteps(current_position);
    }
}

void calibrate() {
    printf("Starting calibration...\n");
    const int num_measurements = 2;
    int total_revolutions = 0;
    int valid_revolutions = 3;

    for (int i = 0; i < num_measurements; i++) {
        int step_count = 0;

        while (gpio_get(OPTOFORK_PIN) != 0) {
            rotate(1);
            step_count++;
            sleep_ms(1);
        }

        while (gpio_get(OPTOFORK_PIN) == 0) {
            rotate(1);
            step_count++;
            sleep_ms(1);
        }

        printf("Counting steps for full rotation...\n");
        while (gpio_get(OPTOFORK_PIN) != 0) {
            rotate(1);
            step_count++;
            sleep_ms(1);
        }
        if (i > 0) {
            total_revolutions += step_count;
        } else {
            printf("Ignored %d %d\n", i+1, step_count);
        }

        printf("Measurement %d: Step count = %d\n", i + 1, step_count);
    }

    steps_per_revolution = total_revolutions / valid_revolutions;
    is_calibrated = true;
    printf("Calibration complete. Average steps per revolution: %d\n", steps_per_revolution);
}

void rotate_steps_512() {
    rotate(512);
}
