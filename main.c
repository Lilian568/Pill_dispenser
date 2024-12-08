#include "led.h"
#include "button.h"
#include "motor.h"
#include "piezo.h"
#include "state.h"
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "lorawan.h"

#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Variables for tracking EEPROM updates
static absolute_time_t last_eeprom_update_time;
static int last_saved_motor_step = -1;

// Function to periodically update EEPROM with motor step information
void maybe_update_eeprom_periodically() {
    absolute_time_t now = get_absolute_time();
    uint64_t diff = absolute_time_diff_us(last_eeprom_update_time, now);

    // Update EEPROM approximately every 50ms (50000 µs)
    if (diff >= 50000) {
        int current_step = get_current_motor_step();
        if (current_step != last_saved_motor_step) {
            DeviceState deviceState;
            if (safe_read_from_eeprom(&deviceState)) {
                deviceState.current_motor_step = current_step;
                if (safe_write_to_eeprom(&deviceState)) {
                    DEBUG_PRINT("Periodic EEPROM update: current_motor_step=%d\n", current_step);
                    last_saved_motor_step = current_step;
                } else {
                    DEBUG_PRINT("EEPROM update failed at step: %d\n", current_step);
                }
            }
        }
        last_eeprom_update_time = now;
    }
}

// Function to send a message over LoRaWAN
void send_lorawan_message(const char *message) {
    printf("Sending LoRaWAN message: %s\n", message);
    char retval_str[256];

    if (!loraMessage(message, strlen(message), retval_str)) {
        printf("[ERROR] Failed to send LoRaWAN message: %s\n", message);
    } else {
        printf("[INFO] LoRaWAN message sent successfully: %s\n", retval_str);
    }
}

int main() {
    // Prevent debug timer from pausing during debugging
    timer_hw->dbgpause = 0;

    // Initialize standard I/O
    stdio_init_all();

    // Initialize EEPROM
    eepromInit();

    // Initialize devices
    ledsInit();
    pwmInit();
    buttonsInit();
    setup();
    setupPiezoSensor();
    loraWanInit();


    // Initialize EEPROM update timer
    last_eeprom_update_time = get_absolute_time();
    last_saved_motor_step = get_current_motor_step();
    // Initialize LoRaWAN
    printf("Initializing LoRaWAN...\n");
    bool lora_connected = false;
    while (!lora_connected) {
        lora_connected = loraInit();
        if (!lora_connected) {
            printf("LoRaWAN connection failed, retrying...\n");
            sleep_ms(5000);
        }
    }
    printf("LoRaWAN connected.\n");
    DeviceState deviceState;

    // Safely load the state from EEPROM
    if (!safe_read_from_eeprom(&deviceState)) {
        DEBUG_PRINT("EEPROM read failed. Setting default state.\n");
        reset_eeprom_internal();
        deviceState.current_state = 1; // Set default state
        deviceState.portion_count = 0;
        deviceState.motor_calibrated = false;
        set_current_motor_step(0);
        last_saved_motor_step = 0;
        safe_write_to_eeprom(&deviceState);
    } else {
        DEBUG_PRINT("State loaded from EEPROM: state=%d, portion_count=%d, motor_calibrated=%d, current_motor_step=%d\n",
                    deviceState.current_state, deviceState.portion_count,
                    deviceState.motor_calibrated, deviceState.current_motor_step);
        set_current_motor_step(deviceState.current_motor_step);
        last_saved_motor_step = deviceState.current_motor_step;
    }

    // If the motor is calibrated, load calibration data and realign
    if (deviceState.motor_calibrated) {
        steps_per_revolution = deviceState.steps_per_revolution;
        steps_per_drop = deviceState.steps_per_drop;
        DEBUG_PRINT("Motor calibratiaon data loaded.\n");
        reset_and_realign();
        maybe_update_eeprom_periodically();
    }

    // Main program states and variables
    bool state1_logged = false;
    bool state2_logged = false;
    absolute_time_t last_time_sw2_pressed = nil_time;
    const uint32_t step_delay_ms = 5000; // Delay between steps in milliseconds
    const int max_portion = 1;           // Maximum portions allowed

    while (true) {
        absolute_time_t current_time = get_absolute_time();

        switch (deviceState.current_state) {
            case 1: // Calibration state
                if (!state1_logged) {
                    send_lorawan_message("Calibration started.");
                    printf("Press SW0 to start calibration.\n");
                    state1_logged = true;
                }

                blink();

                if (isButtonPressed(SW_0)) {
                    calibrate();
                    send_lorawan_message("Calibration completed.");
                    deviceState.current_state = 2; // Move to dispensing state
                    deviceState.motor_calibrated = true;
                    deviceState.steps_per_revolution = steps_per_revolution;
                    deviceState.steps_per_drop = steps_per_drop;
                    deviceState.current_motor_step = get_current_motor_step();
                    deviceState.portion_count = 0;
                    safe_write_to_eeprom(&deviceState);
                    last_saved_motor_step = deviceState.current_motor_step;
                    DEBUG_PRINT("Calibration complete. State saved to EEPROM.\n");

                    state1_logged = false;
                    state2_logged = false;
                    last_time_sw2_pressed = nil_time;
                }
                break;

            case 2: // Dispensing state
                if (!state2_logged) {
                    send_lorawan_message("Dispensing started.");
                    printf("Press SW2 for dispensing.\n");
                    allLedsOn();
                    state2_logged = true;
                }

                uint64_t time_since_last_press = is_nil_time(last_time_sw2_pressed)
                                                 ? 0
                                                 : absolute_time_diff_us(last_time_sw2_pressed, current_time) / 1000;

                if (isButtonPressed(SW_2)) {
                    allLedsOff();
                    printf("Dispensing activated.\n");

                    if (!dispensingAndDetecting()) {
                        send_lorawan_message("Pill not detected during dispensing.");
                        printf("Pill not detected.\n");
                        blinkError(5);
                    } else {
                        send_lorawan_message("Pill detected during dispensing.");
                        printf("Pill dispensed.\n");
                    }

                    deviceState.portion_count++;
                    last_time_sw2_pressed = current_time;

                    deviceState.current_motor_step = get_current_motor_step();
                    safe_write_to_eeprom(&deviceState);
                    last_saved_motor_step = deviceState.current_motor_step;
                    DEBUG_PRINT("State saved to EEPROM after dispensing. Current motor step: %d\n", get_current_motor_step());
                } else if (time_since_last_press >= step_delay_ms && !is_nil_time(last_time_sw2_pressed)) {
                    DEBUG_PRINT("Time delay met. Rotating motor.\n");

                    if (!dispensingAndDetecting()) {
                        send_lorawan_message("Pill not detected during dispensing.");
                        printf("Pill not detected.\n");
                        blinkError(5);
                    } else {
                        send_lorawan_message("Pill detected during dispensing.");
                        printf("Pill dispensed.\n");
                    }

                    deviceState.portion_count++;
                    last_time_sw2_pressed = current_time;

                    deviceState.current_motor_step = get_current_motor_step();
                    safe_write_to_eeprom(&deviceState);
                    last_saved_motor_step = deviceState.current_motor_step;
                    DEBUG_PRINT("State saved to EEPROM after delay dispensing. Current motor step: %d\n", get_current_motor_step());
                }

                if (deviceState.portion_count >= max_portion) {
                    send_lorawan_message("Max portions reached.");
                    DEBUG_PRINT("Max portions reached. Resetting.\n");
                    deviceState.current_state = 1; // Reset to calibration state
                    deviceState.portion_count = 0;
                    deviceState.motor_calibrated = false;
                    safe_write_to_eeprom(&deviceState);
                }
                break;

            default: // Unknown state
                DEBUG_PRINT("Unknown state: %d. Resetting to state 1.\n", deviceState.current_state);
                deviceState.current_state = 1;
                safe_write_to_eeprom(&deviceState);
                break;
        }

        // Periodically update EEPROM
        maybe_update_eeprom_periodically();

        sleep_ms(200);
    }

    return 0;
}
