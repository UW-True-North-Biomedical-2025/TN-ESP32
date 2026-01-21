/* ESP32/I2C/Arduino (slave/receiver) to communicate with Teensy 4.0 

    Sets up ESP32 as I2C receiver that listens for data from Teensy 4.0 transmitter.
    When data is received, it prints the received value to the Serial Monitor.
    
    Connections:
    - ESP32 SDA (GPIO 21) to Teensy SDA
    - ESP32 SCL (GPIO 22) to Teensy SCL
    - Common GND between ESP32 and Teensy
    
    Make sure to install the Wire library if not already included in your Arduino IDE
*/

#include <Wire.h>

#define I2C_ADDR 0x08

volatile byte lastValue = 0;
volatile bool newData = false;

void receiveEvent(int bytes) {
    if (bytes >= 2) {
        byte cmd = Wire.read();
        lastValue = Wire.read();
        newData = true;
    }
}

void requestEvent() {
    Wire.write(lastValue); // echo back
}

void setup() {
    Serial.begin(9600);
    Wire.begin(I2C_ADDR);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

void loop() {
    if (newData) {
        Serial.print("Received value: ");
        Serial.println(lastValue);
        newData = false;
    }
}
