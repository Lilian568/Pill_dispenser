#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct DeviceState {
    int current_state;
    int portion_count;
    bool motor_calibrated;
} DeviceState;

void eepromInit();
void write_to_eeprom(const DeviceState *state);
bool read_from_eeprom(DeviceState *state);

#endif
