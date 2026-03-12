/* ESP32/I2C/Arduino (slave/receiver) to communicate with Teensy 4.0

    Receives float arrays from Teensy (e.g. EMG data). Protocol:
    - Byte 0: number of floats N (1..I2C_MAX_FLOATS)
    - Bytes 1..4*N: N IEEE 754 single-precision floats, little-endian (4 bytes each)

    Connections:
    - ESP32 SDA (GPIO 21) to Teensy SDA
    - ESP32 SCL (GPIO 22) to Teensy SCL
    - Common GND between ESP32 and Teensy
*/

#include <Arduino.h>
#include "i2c_esp32.h"
#include <Wire.h>
#include <string.h>

#define I2C_ADDR 0x08

static float s_floatBuffer[I2C_MAX_FLOATS];
static uint8_t s_floatCount = 0;
static volatile bool s_newData = false;
static uint8_t s_i2cAddress = 0x08;

static void receiveEvent(int bytes)
{
    if (bytes < 1)
        return;
    uint8_t n = Wire.read();
    if (n == 0 || n > I2C_MAX_FLOATS)
        return;
    if (bytes < 1u + 4u * n)
        return;
    s_floatCount = n;
    for (uint8_t i = 0; i < n; i++)
    {
        uint8_t b[4];
        for (int j = 0; j < 4; j++)
            b[j] = Wire.read();
        memcpy(&s_floatBuffer[i], b, 4);
    }
    s_newData = true;
}

static void requestEvent()
{
    // Optionally echo back count + floats if master does request (not required for one-way EMG)
    Wire.write(s_floatCount);
    for (uint8_t i = 0; i < s_floatCount; i++)
    {
        const uint8_t *p = (const uint8_t *)&s_floatBuffer[i];
        for (int j = 0; j < 4; j++)
            Wire.write(p[j]);
    }
}

void i2c_init(uint8_t addr)
{
    s_i2cAddress = addr;
    Wire.begin(s_i2cAddress);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

bool i2c_has_new_data()
{
    return s_newData;
}

uint8_t i2c_get_float_array(float *out)
{
    s_newData = false;
    if (out)
        memcpy(out, s_floatBuffer, (size_t)s_floatCount * sizeof(float));
    return s_floatCount;
}
