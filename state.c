#include "state.h"
#include <stdio.h>
#include <string.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

// EEPROM Configuration
#define I2C_PORT i2c0
#define EEPROM_ADDRESS 0x50
#define BAUDRATE 100000
#define STATE_MEMORY_ADDRESS 0x0000
#define I2C_SDA 16
#define I2C_SCL 17
#define DEVADDR 0x50
#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Mutex for EEPROM operations
mutex_t eeprom_mutex;


// Function to write multiple bytes to EEPROM
void eeprom_write_bytes(uint16_t address, const uint8_t *data, size_t length) {
    uint8_t buffer[2 + length];
    buffer[0] = (address >> 8) & 0xFF;
    buffer[1] = address & 0xFF;
    memcpy(&buffer[2], data, length);
    i2c_write_blocking(I2C_PORT, EEPROM_ADDRESS, buffer, 2 + length, false);
    sleep_ms(5); // Ensure write completes
}

// Safe EEPROM write function with mutex
bool safe_write_to_eeprom(const DeviceState *state) {
    mutex_enter_blocking(&eeprom_mutex); // Lock mutex
    bool success = write_to_eeprom_internal(state);
    mutex_exit(&eeprom_mutex); // Unlock mutex
    return success;
}

// Internal function to write state to EEPROM
bool write_to_eeprom_internal(const DeviceState *state) {
    if (state->current_state < 1 || state->current_state > 2) {
        DEBUG_PRINT("Attempting to write invalid state to EEPROM: state=%d\n", state->current_state);
        return false;
    }

    eeprom_write_bytes(STATE_MEMORY_ADDRESS, (const uint8_t *)state, sizeof(DeviceState));

    DeviceState verify_state;
    if (!read_from_eeprom_internal(&verify_state)) {
        DEBUG_PRINT("Failed to verify EEPROM write.\n");
        return false;
    }

    if (state->current_state != verify_state.current_state ||
        state->portion_count != verify_state.portion_count ||
        state->motor_calibrated != verify_state.motor_calibrated ||
        state->current_motor_step != verify_state.current_motor_step) {
        DEBUG_PRINT("EEPROM write verification failed.\n");
        return false;
    }

    //DEBUG_PRINT("State written to EEPROM successfully: state=%d, portion_count=%d, motor_calibrated=%d, current_motor_step=%d\n",
                //state->current_state, state->portion_count, state->motor_calibrated, state->current_motor_step);
    return true;
}

// Safe EEPROM read function with mutex
bool safe_read_from_eeprom(DeviceState *state) {
    mutex_enter_blocking(&eeprom_mutex); // Lock mutex
    bool success = read_from_eeprom_internal(state);
    mutex_exit(&eeprom_mutex); // Unlock mutex
    return success;
}

// Internal function to read state from EEPROM
bool read_from_eeprom_internal(DeviceState *state) {
    uint8_t buf[2] = {
        (uint8_t)(STATE_MEMORY_ADDRESS >> 8), // Upper byte
        (uint8_t)(STATE_MEMORY_ADDRESS)      // Lower byte
    };

    if (i2c_write_blocking(i2c0, DEVADDR, buf, 2, true) < 0) {
        DEBUG_PRINT("Failed to set EEPROM read address.\n");
        return false;
    }

    if (i2c_read_blocking(i2c0, DEVADDR, (uint8_t *)state, sizeof(DeviceState), false) < 0) {
        DEBUG_PRINT("Failed to read from EEPROM.\n");
        return false;
    }

    if (state->current_state < 1 || state->current_state > 2) {
        DEBUG_PRINT("Unknown state: %d. Resetting to default state.\n", state->current_state);
        reset_eeprom_internal(state);
        return false;
    }

    //DEBUG_PRINT("Read state: state=%d, portion_count=%d, motor_calibrated=%d, current_motor_step=%d\n",
                //state->current_state, state->portion_count, state->motor_calibrated, state->current_motor_step);
    return true;
}

// Reset EEPROM to default state
void reset_eeprom_internal(DeviceState *state) {
    DeviceState default_state = {
        .current_state = 1,
        .portion_count = 0,
        .motor_calibrated = false,
        .steps_per_revolution = 0,
        .steps_per_drop = 0,
        .current_motor_step = 0
    };
    memcpy(state, &default_state, sizeof(DeviceState));
    eeprom_write_bytes(STATE_MEMORY_ADDRESS, (const uint8_t *)state, sizeof(DeviceState));
    DEBUG_PRINT("EEPROM reset to default state.\n");
}

// Initialize EEPROM and mutex
void eepromInit() {
    i2c_init(I2C_PORT, BAUDRATE);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    mutex_init(&eeprom_mutex);
}
