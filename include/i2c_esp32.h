#ifndef ESP32_I2C_H
#define ESP32_I2C_H

#include <Arduino.h>
#include <stdint.h>

// Maximum number of floats in one I2C packet (e.g. 2 for [32, 32], or 4 for 4 EMG channels)
#define I2C_MAX_FLOATS 8

void i2c_init(uint8_t addr = 0x08);

// Check if a new float-array packet has been received
bool i2c_has_new_data();

// Copy the last received float array into `out` (must hold at least I2C_MAX_FLOATS).
// Returns the number of floats received (e.g. 2 for [32, 32]).
// Clears the "new data" flag.
uint8_t i2c_get_float_array(float *out);

#endif // I2C_ESP32_H
