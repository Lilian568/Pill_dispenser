#include <stdio.h>
#include <unistd.h>
#include "pico/stdlib.h"
#include <string.h>
#include "motor.h"
#include <stdlib.h>
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

volatile bool eeprom_write_requested = false; // Flag for EEPROM write requests
volatile DeviceState eeprom_write_buffer;     // Buffer for EEPROM writes

// Returns the current motor step
int get_current_motor_step() {
    return current_motor_step;
}

// Sets the current motor step using modulo operation to handle overflow
void set_current_motor_step(int step) {
    if (steps_per_revolution > 0) {
        current_motor_step = (step % steps_per_revolution + steps_per_revolution) % steps_per_revolution;
    } else {
        current_motor_step = step;
    }
}

// Fine-tunes the motor position after calibration
void fine_tune_position() {
    if (steps_per_revolution > 0) {
        int fine_tune_steps = steps_per_revolution / 13;
        DEBUG_PRINT("Fine-tuning position by %d steps.\n", fine_tune_steps);
        rotate(fine_tune_steps, false);
    }
}

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
    sleep_ms(1); // Delay to stabilize the motor
}

// Motor calibration function
void calibrate() {
    printf("Calibrating motor...\n");

    int step_count = 0;
    int revolutions_total = 0;
    const int valid_iterations = 1; // Number of calibration iterations

    // Perform calibration iterations
    for (int i = 0; i <= valid_iterations; i++) {
        step_count = 0;

        // Rotate until the optical sensor detects a full revolution
        while (gpio_get(OPTOFORK_PIN)) {
            rotate(1, false);
            step_count++;
            sleep_ms(1);
        }
        while (gpio_get(OPTOFORK_PIN) == 0) {
            rotate(1, false);
            step_count++;
            sleep_ms(1);
        }

        // Count steps only after the first iteration
        if (i > 0) {
            revolutions_total += step_count;
        }
    }

    steps_per_revolution = (revolutions_total / valid_iterations);
    steps_per_drop = steps_per_revolution / 8; // Divide steps per revolution into 8 drops
    is_calibrated = true;
    printf("Steps: %d", steps_per_revolution);

    // Align motor to the initial position
    while (gpio_get(OPTOFORK_PIN)) {
        rotate(1, false);
        sleep_ms(1);
    }
    sleep_ms(100);
    while (!gpio_get(OPTOFORK_PIN)) {
        rotate(1, false);
        sleep_ms(1);
    }
    sleep_ms(100);

    fine_tune_position();
    sleep_ms(100);

    current_motor_step = 0;

    // Initialize and save state to EEPROM
    DeviceState deviceState = {1};
    deviceState.current_motor_step = current_motor_step;
    deviceState.steps_per_revolution = steps_per_revolution;
    deviceState.steps_per_drop = steps_per_drop;
    deviceState.motor_calibrated = is_calibrated;

    if (!safe_write_to_eeprom(&deviceState)) {
        DEBUG_PRINT("Calibration: Failed to save state to EEPROM.\n");
    }

    printf("Calibration complete.\n");
}

// Rotate the motor by a given number of steps
void rotate(int step_count, bool update_eeprom) {
    int direction = (step_count > 0) ? 1 : -1; // Determine rotation direction
    step_count = abs(step_count);

    DeviceState deviceState;
    if (update_eeprom) {
        if (!safe_read_from_eeprom(&deviceState)) {
            DEBUG_PRINT("rotate: Failed to read EEPROM before movement.\n");
            return;
        }
        deviceState.in_progress = true;
        deviceState.start_motor_step = current_motor_step;
        safe_write_to_eeprom(&deviceState);
    }

    for (int i = 0; i < step_count; i++) {
        current_motor_step = (current_motor_step + direction + steps_per_revolution) % steps_per_revolution;
        setSteps(current_motor_step % 8);

        // Update EEPROM for each step if requested
        if (update_eeprom) {
            deviceState.current_motor_step = current_motor_step;
            safe_write_to_eeprom(&deviceState);
            DEBUG_PRINT("rotate: Updated EEPROM at step %d.\n", current_motor_step);
        }
    }

    // Finalize the movement and update EEPROM
    if (update_eeprom) {
        deviceState.current_motor_step = current_motor_step;
        deviceState.in_progress = false;
        safe_write_to_eeprom(&deviceState);
        DEBUG_PRINT("rotate: Final EEPROM update. Step: %d, in_progress: false.\n", current_motor_step);
    }
}

// Reset and realign the motor to step 0
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

    if (deviceState.current_motor_step == 0) {
        DEBUG_PRINT("reset_and_realign: Motor already aligned at step 0.\n");
        return;
    }

    int steps_to_reverse = (deviceState.current_motor_step + steps_per_revolution) % steps_per_revolution;

    DEBUG_PRINT("reset_and_realign: Reversing %d steps to realign.\n", steps_to_reverse);
    rotate(-steps_to_reverse, false);

    current_motor_step = 0;
    deviceState.current_motor_step = current_motor_step;
    deviceState.in_progress = false;
    safe_write_to_eeprom(&deviceState);

    DEBUG_PRINT("reset_and_realign: Motor realigned to step 0.\n");
}

// Rotate motor for dispensing one drop (512 steps)
void rotate_steps_512() {
    if (!is_calibrated) {
        calibrate();
    }

    DeviceState deviceState;
    if (safe_read_from_eeprom(&deviceState)) {
        deviceState.in_progress = true;
        deviceState.start_motor_step = current_motor_step;
        safe_write_to_eeprom(&deviceState);

        DEBUG_PRINT("rotate_steps_512: Marking movement as in progress.\n");
    }

    reset_and_realign();
    rotate(steps_per_drop, true);

    if (safe_read_from_eeprom(&deviceState)) {
        deviceState.current_motor_step = current_motor_step;
        deviceState.in_progress = false;
        safe_write_to_eeprom(&deviceState);

        DEBUG_PRINT("rotate_steps_512: Motor step saved to EEPROM. Movement complete.\n");
    }
}

// Setup function to initialize motor and GPIO pins
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

    DEBUG_PRINT("Attempting to read motor state from EEPROM...\n");

    DeviceState deviceState;
    if (!safe_read_from_eeprom(&deviceState)) {
        DEBUG_PRINT("EEPROM read failed. Initializing default state.\n");
        reset_eeprom_internal();
        steps_per_revolution = 512;
        steps_per_drop = steps_per_revolution / 8;
        current_motor_step = 0;
        is_calibrated = false;

        deviceState.steps_per_revolution = steps_per_revolution;
        deviceState.steps_per_drop = steps_per_drop;
        deviceState.current_motor_step = current_motor_step;
        deviceState.motor_calibrated = is_calibrated;
        deviceState.in_progress = false;
        safe_write_to_eeprom(&deviceState);

        DEBUG_PRINT("Default motor state saved to EEPROM.\n");
    } else {
        steps_per_revolution = deviceState.steps_per_revolution;
        steps_per_drop = deviceState.steps_per_drop;
        current_motor_step = deviceState.current_motor_step;
        is_calibrated = deviceState.motor_calibrated;

        DEBUG_PRINT("Motor state loaded: steps_per_revolution=%d, steps_per_drop=%d, current_motor_step=%d\n",
                    steps_per_revolution, steps_per_drop, current_motor_step);
    }

    DEBUG_PRINT("Motor setup complete.\n");
}
