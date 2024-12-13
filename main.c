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


int main() {
    timer_hw->dbgpause = 0;
    stdio_init_all();
    eepromInit();
    ledsInit();
    pwmInit();
    buttonsInit();
    loraWanInit();
    setupPiezoSensor();

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
    send_lorawan_message("BOOT");

    setup();
    last_eeprom_update_time = get_absolute_time();
    last_saved_motor_step = get_current_motor_step();
    DeviceState deviceState;

    // Safely load the state from EEPROM
    if (!safe_read_from_eeprom(&deviceState)) {
        DEBUG_PRINT("EEPROM read failed. Setting default state.\n");
        reset_eeprom_internal();
        deviceState.current_state = 1; // Set default state
        deviceState.portion_count = 0;
        deviceState.motor_calibrated = false;
        deviceState.dispensing_state_active = false;
        set_current_motor_step(0);
        last_saved_motor_step = 0;
        safe_write_to_eeprom(&deviceState);
    } else {
        DEBUG_PRINT(
            "State loaded from EEPROM: state=%d, portion_count=%d, motor_calibrated=%d, current_motor_step=%d\n",
            deviceState.current_state, deviceState.portion_count,
            deviceState.motor_calibrated, deviceState.current_motor_step);
        set_current_motor_step(deviceState.current_motor_step);
        last_saved_motor_step = deviceState.current_motor_step;
    }

    if (deviceState.current_state == 2 && deviceState.dispensing_state_active) {
        printf("Power off during dispensing state, returning to dispensing\n");
        send_lorawan_message("Powered off during dispensing state");
        deviceState.continue_dispensing = true;
    }

    // If the motor is calibrated, load calibration data and realign
    if (deviceState.motor_calibrated) {
        steps_per_revolution = deviceState.steps_per_revolution;
        steps_per_drop = deviceState.steps_per_drop;
    }

    // Main program states and variables
    bool state1_logged = false;
    bool state2_logged = false;
    absolute_time_t last_time_sw2_pressed = nil_time;
    const uint32_t step_delay_ms = 5000; // Delay between steps in milliseconds
    const int max_portion = 7; // Maximum portions allowed

    while (true) {
        absolute_time_t current_time = get_absolute_time();

        switch (deviceState.current_state) {
            case 1: // Calibration state
                if (!state1_logged) {
                    send_lorawan_message("Calibration state.");
                    printf("Press SW0 to start calibration.\n");
                    state1_logged = true;
                }

                blink();

                if (isButtonPressed(SW_0)) {
                    send_lorawan_message("Calibration started.");
                    calibrate();
                    send_lorawan_message("Calibration completed.");
                    deviceState.current_state = 2; // Move to dispensing state
                    deviceState.motor_calibrated = true;
                    deviceState.steps_per_revolution = steps_per_revolution;
                    deviceState.steps_per_drop = steps_per_drop;
                    deviceState.current_motor_step = get_current_motor_step();
                    deviceState.portion_count = 0;
                    deviceState.continue_dispensing = false;
                    safe_write_to_eeprom(&deviceState);
                    last_saved_motor_step = deviceState.current_motor_step;
                    DEBUG_PRINT("Read state: state=%d, portion_count=%d, motor_calibrated=%d, current_motor_step=%d\n",
                                deviceState.current_state, deviceState.portion_count, deviceState.motor_calibrated,
                                deviceState.current_motor_step);

                    state1_logged = false;
                    state2_logged = false;
                    last_time_sw2_pressed = nil_time;
                }
                break;

            case 2: // Dispensing state
                if (!state2_logged) {
                    send_lorawan_message("Dispensing state.");
                    if (!deviceState.continue_dispensing || !deviceState.dispensing_state_active) {
                        printf("Press SW2 for dispensing.\n");
                    }
                    allLedsOn();
                    state2_logged = true;
                }

                uint64_t time_since_last_press = is_nil_time(last_time_sw2_pressed)
                                                     ? 0
                                                     : absolute_time_diff_us(last_time_sw2_pressed,
                                                                             current_time) / 1000;

                if (deviceState.continue_dispensing) {
                    printf("Dispensing after power interruption...\n");

                    if (!dispensingAndDetecting()) {
                        printf("Pill not detected.\n");
                        blinkError(5);

                        deviceState.portion_count++;
                        last_time_sw2_pressed = get_absolute_time();
                        deviceState.continue_dispensing = false;
                        deviceState.motor_was_rotating = false;

                        if (safe_write_to_eeprom(&deviceState)) {
                            send_lorawan_message("Pill not detected during dispensing.");
                        } else {
                            printf("[ERROR] Failed to write to EEPROM.\n");
                        }
                    } else {
                        printf("Pill dispensed.\n");

                        deviceState.portion_count++;
                        last_time_sw2_pressed = get_absolute_time();
                        deviceState.continue_dispensing = false;
                        deviceState.motor_was_rotating = false;

                        if (safe_write_to_eeprom(&deviceState)) {
                            send_lorawan_message("Pill detected during dispensing.");
                        } else {
                            printf("[ERROR] Failed to write to EEPROM.\n");
                        }
                    }


                    deviceState.current_motor_step = get_current_motor_step();
                    safe_write_to_eeprom(&deviceState);
                    last_saved_motor_step = deviceState.current_motor_step;

                    DEBUG_PRINT("Read state: state=%d, portion_count=%d, motor_calibrated=%d, current_motor_step=%d\n",
                                deviceState.current_state, deviceState.portion_count, deviceState.motor_calibrated,
                                deviceState.current_motor_step);
                }

                if (isButtonPressed(SW_2)) {
                    deviceState.dispensing_state_active = true;
                    allLedsOff();
                    printf("Dispensing activated.\n");

                    if (!dispensingAndDetecting()) {
                        printf("Pill not detected.\n");
                        blinkError(5);

                        deviceState.motor_was_rotating = false;
                        deviceState.portion_count++;
                        last_time_sw2_pressed = current_time;
                        deviceState.current_motor_step = get_current_motor_step();

                        if (safe_write_to_eeprom(&deviceState)) {
                            send_lorawan_message("Pill not detected during dispensing.");
                        } else {
                            printf("[ERROR] Failed to write to EEPROM.\n");
                        }
                        last_saved_motor_step = deviceState.current_motor_step;
                    } else {
                        printf("Pill dispensed.\n");

                        deviceState.motor_was_rotating = false;
                        deviceState.portion_count++;
                        last_time_sw2_pressed = current_time;
                        deviceState.current_motor_step = get_current_motor_step();

                        if (safe_write_to_eeprom(&deviceState)) {
                            send_lorawan_message("Pill detected during dispensing.");
                        } else {
                            printf("[ERROR] Failed to write to EEPROM.\n");
                        }
                        last_saved_motor_step = deviceState.current_motor_step;
                    }
                    DEBUG_PRINT("Read state: state=%d, portion_count=%d, motor_calibrated=%d, current_motor_step=%d\n",
                                deviceState.current_state, deviceState.portion_count, deviceState.motor_calibrated,
                                deviceState.current_motor_step);
                } else if (time_since_last_press >= step_delay_ms && !is_nil_time(last_time_sw2_pressed)) {
                    printf("Time delay met. Dispensing next pill.\n");

                    if (!dispensingAndDetecting()) {
                        printf("Pill not detected.\n");
                        blinkError(5);
                        deviceState.motor_was_rotating= false;
                        deviceState.portion_count++;
                        last_time_sw2_pressed = current_time;
                        deviceState.current_motor_step = get_current_motor_step();

                        if (safe_write_to_eeprom(&deviceState)) {
                            send_lorawan_message("Pill not detected during dispensing.");
                        } else {
                            printf("[ERROR] Failed to write to EEPROM.\n");
                        }
                        last_saved_motor_step = deviceState.current_motor_step;
                    } else {
                        printf("Pill dispensed.\n");
                        deviceState.motor_was_rotating = false;
                        deviceState.portion_count++;
                        last_time_sw2_pressed = current_time;
                        deviceState.current_motor_step = get_current_motor_step();

                        if (safe_write_to_eeprom(&deviceState)) {
                            send_lorawan_message("Pill detected during dispensing.");
                        } else {
                            printf("[ERROR] Failed to write to EEPROM.\n");
                        }
                        last_saved_motor_step = deviceState.current_motor_step;
                    }
                    DEBUG_PRINT("Read state: state=%d, portion_count=%d, motor_calibrated=%d, current_motor_step=%d\n",
                                deviceState.current_state, deviceState.portion_count, deviceState.motor_calibrated,
                                deviceState.current_motor_step);
                }

                if (deviceState.portion_count >= max_portion) {
                    printf("Max portions reached. Resetting.\n");
                    send_lorawan_message("Max portions reached.");
                    deviceState.current_state = 1; // Reset to calibration state
                    deviceState.portion_count = 0;
                    deviceState.motor_calibrated = false;
                    deviceState.motor_was_rotating = false;
                    deviceState.dispensing_state_active = false;
                    safe_write_to_eeprom(&deviceState);
                }
                break;

            default: // Unknown state
                DEBUG_PRINT("Unknown state: %d. Resetting to state 1.\n", deviceState.current_state);
                deviceState.current_state = 1;
                safe_write_to_eeprom(&deviceState);
                DEBUG_PRINT("Read state: state=%d, portion_count=%d, motor_calibrated=%d, current_motor_step=%d\n",
                            deviceState.current_state, deviceState.portion_count, deviceState.motor_calibrated,
                            deviceState.current_motor_step);
                break;
        }


        sleep_ms(200);
    }

    return 0;
}
