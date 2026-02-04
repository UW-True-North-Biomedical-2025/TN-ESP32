// #include <Arduino.h>

// char data = 0;    
// bool status = false;        //Variable for storing received data
// void  setup()
// {
//     Serial.begin(115200);   //Sets the baud for serial data transmission
//     delay(1000);          //Wait for serial connection to establish
    
//     // Test if ESP is connected
//     Serial.println("ESP32 Connected and Ready!");
//     Serial.println("Waiting for commands...");
//     Serial.println("Send '1' to turn LED ON, '0' to turn LED OFF");
    
//     pinMode(2, OUTPUT);  //Sets digital pin  13 as output pin
    
//     // Blink LED to indicate successful connection
//     for(int i = 0; i < 3; i++) {
//         digitalWrite(2, HIGH);
//         delay(200);
//         digitalWrite(2, LOW);
//         delay(200);
//     }
// }
// void loop() {
//     if(Serial.available() > 0) {
//         data = Serial.read();
//         if(data == '1')
//             digitalWrite(2, HIGH);
//         else if(data == '0')
//             digitalWrite(2, LOW);
//     }
// }




// //    if(Serial.available() > 0)      //  Send data only when you receive data:
// //    
// //       data = Serial.read();        //Read  the incoming data & store into data
// //       Serial.print(data);          //Print  Value inside data in Serial monitor
// //       Serial.print("\
// // ");        
//       // if(status == true)              // Checks whether value of data is equal to 1
//       //    digitalWrite(2, HIGH);   //If value is 1 then LED turns ON
//       // else  if(status == false)         //  Checks whether value of data is equal to 0
//       //    digitalWrite(2,  LOW);    //If value is 0 then LED turns OFF
   




#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "I2C_ESP32.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  i2c_init(0x08);  // Initialize I2C with address 0x08

  // BLEDevice::init("MyESP32");  // set the device name
  // BLEServer *pServer = BLEDevice::createServer();
  // BLEService *pService = pServer->createService(SERVICE_UUID);
  // BLECharacteristic *pCharacteristic = pService->createCharacteristic(
  //                                        CHARACTERISTIC_UUID,
  //                                        BLECharacteristic::PROPERTY_READ |
  //                                        BLECharacteristic::PROPERTY_WRITE
  //                                      );

  // pCharacteristic->setValue("Hello World!");
  // pService->start();
  // // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  // BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  // pAdvertising->addServiceUUID(SERVICE_UUID);
  // pAdvertising->setScanResponse(true);
  // pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  // pAdvertising->setMinPreferred(0x12);
  // BLEDevice::startAdvertising();
  // Serial.println("Characteristic defined! Now you can read it in your phone!");

  

}

void loop() {
  // put your main code here, to run repeatedly:

//   float counter = 0.0;
// // Testing multi-data print (see results in platformio monitor)

//     // float x_value[counter];
//     // x_value[0] = 0.0;

//     // for ( = 1; i < 50; i++) {
//     // / x_value[i] = x_value[i-1] +0.1;}
//     while (true){
//        delay(500);
//         float y_1 = 5 * sin(2 * PI * counter);
//         float y_2 = 3 * sin(2 * PI * counter - 4.0);
//         float y_3 = sin(2 * PI * counter - 8.0);
//         float y_4 = 0.5 * sin(2 * PI * counter - 12.0);

//         printf("EMG: %.2f, y1: %.2f, y2: %.2f, y3: %.2f, y4: %.2f\n", counter, y_1, y_2, y_3, y_4);
//         counter+=0.1;
//     }

  if (i2c_has_new_data()) { // Check if new I2C data has been received
      Serial.print("Received value: "); 
      Serial.println(i2c_get_last_value());
  }
  delay(10); 
}