#include <Wire.h>

#define I2C_ADDR 0x08

volatile byte lastCommand = 0;

void receiveEvent(int bytes) {
  if (Wire.available()) {
    lastCommand = Wire.read();
  }
}

void requestEvent() {
  if (lastCommand == 0x01) {
    Wire.write(0xAA);  // status byte
  } else {
    Wire.write(0x00);
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin(I2C_ADDR);  // Slave
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

void loop() {
  // Keep loop light — callbacks do the work
}
