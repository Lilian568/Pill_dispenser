#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <pico/mutex.h>

// Structure to store the device's state
typedef struct DeviceState {
    int current_state;          // Current state of the device (e.g., idle, dispensing)
    int portion_count;          // Number of dispensed portions
    bool motor_calibrated;      // Motor calibration status
    int steps_per_revolution;   // Steps required for a full motor revolution
    int steps_per_drop;         // Steps required for dispensing one portion
    int current_motor_step;     // Current step position of the motor
    int fine_tune_steps;        // Fine-tuning steps for alignment
    int start_motor_step;       // Marking starting step
    bool motor_was_rotating;    // Indicator if motor was rotating
    bool calibrating;           // Indicator if motor was in middle of calibration
    bool continue_dispensing;   // Indicator for continuing dispensing after reset
} DeviceState;

// External mutex for EEPROM operations
extern mutex_t eeprom_mutex;

// Public function prototypes
void eepromInit();                                // Initialize the EEPROM and related configurations
bool write_to_eeprom(const DeviceState *state);   // Write device state to EEPROM (thread-safe)
bool read_from_eeprom(DeviceState *state);        // Read device state from EEPROM (thread-safe)
void reset_eeprom();                              // Reset EEPROM to the default state
// Safe functions for thread-safe EEPROM operations
bool safe_write_to_eeprom(const DeviceState *state); // Mutex write
bool safe_read_from_eeprom(DeviceState *state);     // Mutex read
bool write_to_eeprom_internal(const DeviceState *state); // Internal write implementation
bool read_from_eeprom_internal(DeviceState *state);      // Internal read implementation
void reset_eeprom_internal();                           // Function for eeprom reset
uint16_t crc16(const uint8_t *data_p, size_t length);  // Checksum

#endif // STATE_H
