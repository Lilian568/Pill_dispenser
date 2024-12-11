
#include <unistd.h>
#include "pico/stdlib.h"
#include <string.h>
#include "motor.h"

#include <stdio.h>
#include <stdlib.h>

#include "lorawan.h"
#include "state.h"

#define OPTOFORK_PIN 28
#define MOTOR_PIN1 2
#define MOTOR_PIN2 3
#define MOTOR_PIN3 6
#define MOTOR_PIN4 13

#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Global variables for motor control
int steps_per_revolution = -1; // Number of steps per revolution
int steps_per_drop = -1;       // Number of steps per pill drop
int current_motor_step = 0;    // Current motor step
bool is_calibrated = false;    // Calibration status

// Sets the motor step by activating the corresponding GPIO pins
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

// Returns the current motor step
int get_current_motor_step() {
    return current_motor_step;
}

// Sets the current motor step
void set_current_motor_step(int step) {
    if (steps_per_revolution > 0) {
        current_motor_step = (step % steps_per_revolution + steps_per_revolution) % steps_per_revolution;
    } else {
        current_motor_step = step;
    }
}

// Rotate the motor by a given number of steps
void rotate(int step_count, bool update_eeprom) {
    int direction = (step_count > 0) ? 1 : -1;
    step_count = abs(step_count);

    for (int i = 0; i < step_count; i++) {
        current_motor_step = (current_motor_step + direction + steps_per_revolution) % steps_per_revolution;
        setSteps(current_motor_step % 8);
    }

    if (update_eeprom) {
        DeviceState deviceState;
        if (safe_read_from_eeprom(&deviceState)) {
            deviceState.current_motor_step = current_motor_step;
            deviceState.motor_was_rotating = true; // Mark motor as rotating
            safe_write_to_eeprom(&deviceState);
        }
    }
}

// Fine-tunes the motor position after calibration
void fine_tune_position() {
    if (steps_per_revolution > 0) {
        int fine_tune_steps = steps_per_revolution / 13;
        rotate(fine_tune_steps, false);
    }
}
void fine_tune_position_2() {
    if (steps_per_revolution > 0) {
        int fine_tune_steps = steps_per_revolution / 12;
        rotate(fine_tune_steps, false);
    }
}

// Motor calibration function
void calibrate() {
    printf("Calibrating motor...\n");

    DeviceState deviceState;
    if (safe_read_from_eeprom(&deviceState)) {
        deviceState.calibrating = true;
        safe_write_to_eeprom(&deviceState);
    }

    int step_count = 0;
    int revolutions_total = 0;
    const int valid_iterations = 2;

    for (int i = 0; i <= valid_iterations; i++) {
        step_count = 0;

        while (gpio_get(OPTOFORK_PIN)) {
            rotate(1, false);
            step_count++;
            sleep_ms(1);
        }
        sleep_ms(100);
        while (!gpio_get(OPTOFORK_PIN)) {
            rotate(1, false);
            step_count++;
            sleep_ms(1);
        }

        if (i > 0) {
            revolutions_total += step_count;
        }
    }

    steps_per_revolution = (revolutions_total / valid_iterations);
    steps_per_drop = steps_per_revolution / 8;
    is_calibrated = true;



    fine_tune_position();
    sleep_ms(100);
    printf("Calibration complete.\n");

    current_motor_step = 0;



    // Mark calibration as complete
    if (safe_read_from_eeprom(&deviceState)) {
        deviceState.motor_calibrated = true;
        deviceState.current_state = 2;
        deviceState.steps_per_revolution = steps_per_revolution;
        deviceState.steps_per_drop = steps_per_drop;
        deviceState.current_motor_step = 0;
        deviceState.calibrating = false;
        deviceState.continue_dispensing = false;
        safe_write_to_eeprom(&deviceState);
    }
}

// Reset and realign the motor
void reset_and_realign() {
    if (!is_calibrated) {
        DEBUG_PRINT("reset_and_realign: Motor not calibrated.\n");
        return;
    }

    DeviceState deviceState;
    if (!safe_read_from_eeprom(&deviceState)) {
        DEBUG_PRINT("reset_and_realign: EEPROM read failed.\n");
        return;
    }

    while (gpio_get(OPTOFORK_PIN) == 1) {
        rotate(-1, false);
        sleep_ms(1);
    }
        fine_tune_position_2();

    DEBUG_PRINT("Adjusting the sensor back correct step: %d\n", deviceState.current_motor_step);
    rotate(deviceState.current_motor_step, false);

    current_motor_step = deviceState.current_motor_step;
    if (safe_read_from_eeprom(&deviceState)) {
        deviceState.motor_was_rotating = false; // Calibration complete, motor not rotating
        safe_write_to_eeprom(&deviceState);
    }
    safe_write_to_eeprom(&deviceState);

}

// Rotate motor for dispensing one drop
void rotate_steps_512() {
    if (!is_calibrated) {
        calibrate();
    }

    DeviceState deviceState;
    if (safe_read_from_eeprom(&deviceState)) {
        deviceState.motor_was_rotating = true; // Mark motor as rotating
        safe_write_to_eeprom(&deviceState);
    }

    rotate(steps_per_drop, true);

    if (safe_read_from_eeprom(&deviceState)) {
        deviceState.motor_was_rotating = false; // Mark motor as not rotating
        safe_write_to_eeprom(&deviceState);
    }

}

// Setup function to initialize device
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

    DeviceState deviceState;
    if (!safe_read_from_eeprom(&deviceState)) {
        DEBUG_PRINT("EEPROM read failed. Initializing default state.\n");
        steps_per_revolution = 512;
        steps_per_drop = steps_per_revolution / 8;
        current_motor_step = 0;
        is_calibrated = false;

        deviceState.steps_per_revolution = steps_per_revolution;
        deviceState.steps_per_drop = steps_per_drop;
        deviceState.current_motor_step = current_motor_step;
        deviceState.motor_calibrated = is_calibrated;
        deviceState.motor_was_rotating = false;
        deviceState.calibrating = false;
        safe_write_to_eeprom(&deviceState);

        DEBUG_PRINT("Default motor state saved to EEPROM.\n");
    } else {
        steps_per_revolution = deviceState.steps_per_revolution;
        steps_per_drop = deviceState.steps_per_drop;
        current_motor_step = deviceState.current_motor_step;
        is_calibrated = deviceState.motor_calibrated;

        if (deviceState.calibrating) {
            DEBUG_PRINT("Device was turned while calibrating. Restarting calibration...\n");
            send_lorawan_message("Device was turned off in calibration state");
            calibrate();
        } else if (deviceState.motor_was_rotating) {
            DEBUG_PRINT("Device was turned off while motor was rotating. Realigning...\n");
            send_lorawan_message("Device was turned off in dispensing state.");
            reset_and_realign();
            deviceState.continue_dispensing = true;
            safe_write_to_eeprom(&deviceState);
        }

        DEBUG_PRINT("Motor setup complete. Steps per revolution: %d, Steps per drop: %d, Current step: %d\n",
                    steps_per_revolution, steps_per_drop, current_motor_step);
    }
}