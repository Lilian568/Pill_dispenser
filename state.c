#include "state.h"
#include <string.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define I2C_SDA 16
#define I2C_SCL 17
#define DEVADDR 0x50
#define BAUDRATE 100000
#define I2C_MEMORY_SIZE 32768
#define STATE_MEMORY_ADDRESS 0x0000

#ifdef DEBUG_PRINT
#define DEBUG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DEBUG_PRINT(f_, ...)
#endif


extern int * log_counter;


// eeprom function
void eepromInit() {
    i2c_init(i2c0, BAUDRATE);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void write_to_eeprom(const DeviceState *state) {
    DeviceState stateToWrite = *state;

    uint16_t crc = crc16((uint8_t *) &stateToWrite, sizeof(stateToWrite)-sizeof(stateToWrite.crc16));
    stateToWrite.crc16 = crc;

    uint16_t write_address = I2C_MEMORY_SIZE - sizeof(stateToWrite);
    uint8_t *buffer = (uint8_t *) &stateToWrite;
    eepromWriteBytes(write_address, buffer, sizeof(stateToWrite));
}

bool read_from_eeprom(DeviceState *state) {
    DeviceState stateToRead;
    uint16_t read_address = I2C_MEMORY_SIZE - sizeof(stateToRead);
    eepromReadBytes(read_address, (uint8_t*) &stateToRead, sizeof(stateToRead));
    uint16_t calc_crc16 = crc16((uint8_t*)&stateToRead,sizeof(stateToRead) -sizeof(stateToRead.crc16));

    if (stateToRead.crc16 == calc_crc16) {
        memcpy(state, &stateToRead, sizeof(stateToRead));
        return true;
    } else {
        return false;
    }
}


void eepromWriteBytes(uint16_t address, const uint8_t *data, uint8_t length) {
    assert(data != NULL);
    assert(address < I2C_MEMORY_SIZE);
    assert(0 < length);
    assert(length <= I2C_MEM_PAGE_SIZE);
    assert((address / I2C_MEM_PAGE_SIZE) == ((address + length - 1) / I2C_MEM_PAGE_SIZE));

    uint8_t buffer[length+2];
    buffer[0] = address >> 8; buffer[1] = address;
    memcpy( &buffer[2], data, length);
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
    sleep_ms(I2C_MEM_WRITE_TIME);
}


void eepromWriteByte_NoDelay(uint16_t address, uint8_t data) {
    assert(address < I2C_MEMORY_SIZE);

    uint8_t buffer[3];
    buffer[0] = address >> 8; buffer[1] = address; buffer[2] = data;
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
}


void eepromWriteByte(uint16_t address, uint8_t data) {
    assert(address < I2C_MEMORY_SIZE);

    uint8_t buffer[3];
    buffer[0] = address >> 8; buffer[1] = address; buffer[2] = data;
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
    sleep_ms(I2C_MEM_WRITE_TIME);
}



uint8_t eepromReadByte(uint16_t address) {
    assert(address < I2C_MEMORY_SIZE);

    uint8_t buffer[2];
    buffer[0] = address >> 8; buffer[1] = address;
    i2c_write_blocking(i2c0, DEVADDR, buffer, 2, true);
    i2c_read_blocking(i2c0, DEVADDR, buffer, 1, false);
    return buffer[0];
}

void eepromReadBytes(uint16_t address, uint8_t *data, uint8_t length) {
    assert(data != NULL);
    assert(address < I2C_MEMORY_SIZE);
    assert(0 < length);

    uint8_t buffer[2];
    buffer[0] = address >> 8; buffer[1] = address;
    i2c_write_blocking(i2c0, DEVADDR, buffer, 2, true);
    i2c_read_blocking(i2c0, DEVADDR, data, length, false);
}

uint16_t crc16(const uint8_t *data, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--) {
        x = crc >> 8 ^ *data++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t) (x << 12)) ^ ((uint16_t) (x << 5)) ^ ((uint16_t) (x));
    }
    return crc;
}


void writeLogEntry(const char *message) {
    if (*log_counter >= MAX_LOG_ENTRY) {
        DEBUG_PRINT("Maximum allowed log number reached. ");
        eraseLog();
    }

    size_t message_length = strlen(message);
    if (message_length >= 1) {
        if (message_length > 61) {
            message_length = 61;
        }

        uint8_t buffer[message_length + 3];
        memcpy(buffer, message, message_length);
        buffer[message_length] = '\0';

        uint16_t crc = crc16(buffer, message_length + 1);
        buffer[message_length + 1] = (uint8_t) (crc >> 8);
        buffer[message_length + 2] = (uint8_t) crc;

        uint16_t log_address = MAX_LOG_SIZE * *log_counter;
        *log_counter = *log_counter + 1;
        eepromWriteBytes(log_address, buffer, message_length+3);
    } else {
        DEBUG_PRINT("Invalid input. Log message must contain at least one character.\n");
    }
}


void printLog() {
    if (0 != *log_counter) {
        uint16_t log_address = MEM_ADDR_START;
        uint8_t buffer[MAX_LOG_SIZE];

        DEBUG_PRINT("Printing log messages from memory:\n");
        for (int i = 0; i < *log_counter; i++) {
            log_address = i * MAX_LOG_SIZE;
            eepromReadBytes(log_address, buffer, MAX_LOG_SIZE);

            int term_zero_index = 0;
            while (buffer[term_zero_index] != '\0') {
                term_zero_index++;
            }

            if(0 == crc16(buffer, (term_zero_index + 3)) && buffer[0] != 0 && (term_zero_index < (MAX_LOG_SIZE - 2))) {
                DEBUG_PRINT("Log #%d: ", i + 1);
                int index = 0;
                while (buffer[index]) {
                    DEBUG_PRINT("%c", buffer[index++]);
                }
                DEBUG_PRINT("\n");
            } else {
                DEBUG_PRINT("Log message #%d invalid. Exit printing.\n", i + 1);
                break;
            }
        }
    } else {
        DEBUG_PRINT("No log message in memory yet.\n");
    }
}


void eraseLog() {
    DEBUG_PRINT("Erasing log messages from memory:\n");
    uint16_t log_address = 0;
    for (int i = 0; i < MAX_LOG_ENTRY; i++) {
        eepromWriteByte(log_address, 0);
        log_address += MAX_LOG_SIZE;
    }
    *log_counter = 0;
    DEBUG_PRINT("All done.\n");
}


void eraseAll() {
    DEBUG_PRINT("Erasing all messages from memory:\n");
    uint16_t log_address = 0;
    for (int i = 0; i < MAX_LOG_ENTRY*MAX_LOG_SIZE; i++) {
        eepromWriteByte(log_address, 0xFF);
        log_address ++;
    }
    DEBUG_PRINT("All done.\n");
}



void printAllMemory() {
    DEBUG_PRINT("Printing all messages from memory:\n");
    for (int i = 0; i < MAX_LOG_ENTRY; i++) {
        for (int j = 0; j<MAX_LOG_ENTRY; j ++) {
            uint8_t printed = eepromReadByte(i * MAX_LOG_SIZE + j);
            DEBUG_PRINT("%x", printed);
        }
        DEBUG_PRINT("\n");
    }
    DEBUG_PRINT("\n");
}
