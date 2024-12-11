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

// Internal function for writing
bool write_to_eeprom_internal(const DeviceState *state) {
    if (state->current_state < 1 || state->current_state > 2) {
        DEBUG_PRINT("Attempting to write invalid state to EEPROM: state=%d\n", state->current_state);
        return false;
    }

    uint16_t checksum = crc16((const uint8_t *)state, sizeof(DeviceState));

    uint8_t buffer[sizeof(DeviceState) + sizeof(uint16_t)];
    memcpy(buffer, state, sizeof(DeviceState));
    memcpy(buffer + sizeof(DeviceState), &checksum, sizeof(uint16_t));

    eeprom_write_bytes(STATE_MEMORY_ADDRESS, buffer, sizeof(buffer));
    sleep_ms(200);

    uint8_t verify_buffer[sizeof(DeviceState) + sizeof(uint16_t)];
    uint8_t addr[2] = {
        (STATE_MEMORY_ADDRESS >> 8) & 0xFF,
        STATE_MEMORY_ADDRESS & 0xFF
    };
    i2c_write_blocking(i2c0, DEVADDR, addr, 2, true); // Aseta osoite
    if (i2c_read_blocking(i2c0, DEVADDR, verify_buffer, sizeof(verify_buffer), false) < 0) {
        DEBUG_PRINT("Failed to verify EEPROM write.\n");
        return false;
    }

    if (memcmp(buffer, verify_buffer, sizeof(buffer)) != 0) {
        DEBUG_PRINT("EEPROM write verification failed.\n");
        for (size_t i = 0; i < sizeof(buffer); i++) {
            printf("[DEBUG] Written byte: 0x%02X, Read byte: 0x%02X\n", buffer[i], verify_buffer[i]);
        }
        return false;
    }

    DEBUG_PRINT("State and checksum written to EEPROM successfully.\n");
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
    uint8_t buffer[sizeof(DeviceState) + sizeof(uint16_t)];

    uint8_t address_bytes[2] = {
        (uint8_t)(STATE_MEMORY_ADDRESS >> 8), // Upper byte
        (uint8_t)(STATE_MEMORY_ADDRESS)      // Lower byte
    };
    if (i2c_write_blocking(i2c0, DEVADDR, address_bytes, 2, true) < 0 ||
        i2c_read_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false) < 0) {
        DEBUG_PRINT("Failed to read from EEPROM.\n");
        return false;
        }

    memcpy(state, buffer, sizeof(DeviceState));
    uint16_t stored_checksum;
    memcpy(&stored_checksum, buffer + sizeof(DeviceState), sizeof(uint16_t));

    uint16_t calculated_checksum = crc16((const uint8_t *)state, sizeof(DeviceState));

    if (stored_checksum != calculated_checksum) {
        DEBUG_PRINT("Checksum mismatch. Data may be corrupted.\n");
        return false;
    }

    DEBUG_PRINT("Checksum validated. Data is intact.\n");
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

    uint16_t checksum = crc16((const uint8_t *)&default_state, sizeof(DeviceState));
    uint8_t buffer[sizeof(DeviceState) + sizeof(uint16_t)];
    memcpy(buffer, &default_state, sizeof(DeviceState));
    memcpy(buffer + sizeof(DeviceState), &checksum, sizeof(uint16_t));
    eeprom_write_bytes(STATE_MEMORY_ADDRESS, buffer, sizeof(buffer));
    memcpy(state, &default_state, sizeof(DeviceState));
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

// Checksum calculations
uint16_t crc16(const uint8_t *data_p, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;
    while (length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
    }
    return crc;
}