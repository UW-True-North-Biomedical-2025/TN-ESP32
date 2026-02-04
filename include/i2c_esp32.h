// declarations

#ifndef ESP32_I2C_H
#define ESP32_I2C_H

#include <Arduino.h>
#include <stdint.h>

// functions
void i2c_init(uint8_t addr = 0x08);
bool i2c_has_new_data();
uint8_t i2c_get_last_value();

#endif // I2C_ESP32_H