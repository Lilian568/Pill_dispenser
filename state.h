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
    int optofork_step;          // Step where the optofork sensor activates
    int fine_tune_steps;        // Fine-tuning steps for alignment
    uint32_t checksum;          // Checksum for data integrity
    bool in_progress;           // Indicates if the motor is in progress
    int start_motor_step;       // Start step of the motor's current operation
} DeviceState;

// External mutex for EEPROM operations
extern mutex_t eeprom_mutex;

// Public function prototypes
void eepromInit();                                // Initialize the EEPROM and related configurations
bool write_to_eeprom(const DeviceState *state);   // Write device state to EEPROM (thread-safe)
bool read_from_eeprom(DeviceState *state);        // Read device state from EEPROM (thread-safe)
void reset_eeprom();                              // Reset EEPROM to the default state
uint32_t calculate_checksum(const DeviceState *state); // Calculate checksum for data integrity
// Safe functions for thread-safe EEPROM operations
bool safe_write_to_eeprom(const DeviceState *state);
bool safe_read_from_eeprom(DeviceState *state);
// Internal function prototypes (used only within `state.c`)
bool write_to_eeprom_internal(const DeviceState *state); // Internal write implementation
bool read_from_eeprom_internal(DeviceState *state);      // Internal read implementation
void reset_eeprom_internal();                            // Internal reset implementation

#endif // STATE_H
