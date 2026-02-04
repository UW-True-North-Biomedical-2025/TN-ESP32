/* ESP32/I2C/Arduino (slave/receiver) to communicate with Teensy 4.0 

    Sets up ESP32 as I2C receiver that listens for data from Teensy 4.0 transmitter.
    When data is received, it prints the received value to the Serial Monitor.
    
    Connections:
    - ESP32 SDA (GPIO 21) to Teensy SDA
    - ESP32 SCL (GPIO 22) to Teensy SCL
    - Common GND between ESP32 and Teensy
    
    Make sure to install the Wire library if not already included in your Arduino IDE
*/

// #include <Arduino.h>
#include "I2C_ESP32.h"
#include <Wire.h>

// #define I2C_ADDR 0x08

static volatile uint8_t s_lastValue = 0;
static volatile bool s_newData = false;
static uint8_t s_i2cAddress = 0x08;

static void receiveEvent(int bytes) {
    if (bytes >= 2) {
        Wire.read(); // discard command byte
        s_lastValue = Wire.read();
        s_newData = true;
    }
}

static void requestEvent() {
    Wire.write(s_lastValue); // echo back
}

void i2c_init(uint8_t addr) {
    s_i2cAddress = addr;
    Wire.begin(s_i2cAddress);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

bool i2c_has_new_data() {
    return s_newData;
}

uint8_t i2c_get_last_value() {
    s_newData = false; // reset new data flag
    return s_lastValue;
}