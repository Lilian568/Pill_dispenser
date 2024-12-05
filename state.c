#include "state.h"
#include <string.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define I2C_SDA 16
#define I2C_SCL 17
#define DEVADDR 0x50
#define BAUDRATE 100000
#define I2C_MEMORY 32768
#define STATE_MEMORY_ADDRESS 0x0000


void eepromInit() {
    i2c_init(i2c0, BAUDRATE);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void write_to_eeprom(const DeviceState *state) {
    uint8_t data[sizeof(DeviceState)];
    memcpy(data, state, sizeof(DeviceState));
    uint8_t buf[2 + sizeof(DeviceState)];
    buf[0] = (uint8_t)(STATE_MEMORY_ADDRESS >> 8);
    buf[1] = (uint8_t)(STATE_MEMORY_ADDRESS);
    memcpy(&buf[2], data, sizeof(DeviceState));
    i2c_write_blocking(i2c0, DEVADDR, buf, sizeof(buf), false);
    sleep_ms(5);
}

bool read_from_eeprom(DeviceState *state) {
    uint8_t buf[2];
    buf[0] = (uint8_t)(STATE_MEMORY_ADDRESS >> 8);
    buf[1] = (uint8_t)(STATE_MEMORY_ADDRESS);

    if (i2c_write_blocking(i2c0, DEVADDR, buf, 2, true) < 0) {
        return false;
    }

    if (i2c_read_blocking(i2c0, DEVADDR, (uint8_t *)state, sizeof(DeviceState), false) < 0) {
        return false;
    }

    return true;
}
