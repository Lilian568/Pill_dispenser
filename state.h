#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <stdbool.h>
#include<stdio.h>

/*   I2C   */
#define I2C_MEM_PAGE_SIZE 64
#define I2C_MEM_WRITE_TIME 10
#define MEM_ADDR_START 0
#define MAX_LOG_SIZE 64
#define MAX_LOG_ENTRY 32
#define STEPPER_POSITION_ADDRESS  ( I2C_MEMORY_SIZE / 2 )


enum SystemState {
    CALIB_WAITING,       // EEPROM, CALIBRATED: 0 == CALIB_WAITING
    DISPENSE_WAITING     //                     1 == DISPENSE_WAITING
};

enum CompartmentState {
    IN_THE_MIDDLE,      // EEPROM, 0 == IN_THE_MIDDLE
    FINISHED            //         1 == FINISHED
};

typedef struct DeviceState {
    enum SystemState current_state;
    enum CompartmentState compartmentFinished;
    int logCounter;
    int portion_count;
    bool motor_calibrated;
    int calibrationCount;
    int compartmentsMoved;
    uint16_t crc16;
} DeviceState;

void eepromInit();
void write_to_eeprom(const DeviceState *state);
bool read_from_eeprom(DeviceState *state);
void eepromWriteBytes(uint16_t address, const uint8_t *data, uint8_t length);
void eepromWriteByte_NoDelay(uint16_t address, uint8_t data);
void eepromWriteByte(uint16_t address, uint8_t data);
uint8_t eepromReadByte(uint16_t address);
void eepromReadBytes(uint16_t address, uint8_t *data, uint8_t length);
uint16_t crc16(const uint8_t *data, size_t length);
void writeLogEntry(const char *message);
void printLog();
void eraseLog();
void printAllMemory();
void eraseAll();

#endif
