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
#include <BLE2902.h>

// UUIDs for BLE service and characteristics
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define EMG1_CHAR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define EMG2_CHAR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define EMG3_CHAR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define EMG4_CHAR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26ab"

BLECharacteristic *pCharEmg1;
BLECharacteristic *pCharEmg2;
BLECharacteristic *pCharEmg3;
BLECharacteristic *pCharEmg4;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// Callback for connection events
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Client connected!");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client disconnected!");
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE EMG Streaming!");

  BLEDevice::init("MyESP32");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create 4 characteristics with NOTIFY property
  pCharEmg1 = pService->createCharacteristic(
    EMG1_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharEmg1->addDescriptor(new BLE2902());
  
  pCharEmg2 = pService->createCharacteristic(
    EMG2_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharEmg2->addDescriptor(new BLE2902());
  
  pCharEmg3 = pService->createCharacteristic(
    EMG3_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharEmg3->addDescriptor(new BLE2902());
  
  pCharEmg4 = pService->createCharacteristic(
    EMG4_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharEmg4->addDescriptor(new BLE2902());

  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE ready! Waiting for connection...");
}

void loop() {
  // Handle reconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give time for BLE stack to get ready
    BLEDevice::startAdvertising();
    Serial.println("Start advertising for reconnection...");
    oldDeviceConnected = deviceConnected;
  }
  
  // Handle new connection
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    Serial.println("Ready to stream data!");
  }
  
  // Only send data when connected
  if (deviceConnected) {
    static float counter = 0.0;
    
    // Generate 4 sine wave signals (simulating EMG data)
    float y_1 = 2048 + 500 * sin(2 * PI * counter);
    float y_2 = 2048 + 400 * sin(2 * PI * counter - 1.0);
    float y_3 = 2048 + 300 * sin(2 * PI * counter - 2.0);
    float y_4 = 2048 + 200 * sin(2 * PI * counter - 3.0);
    
    // Convert to uint16_t (0-4095 range like 12-bit ADC)
    uint16_t val1 = (uint16_t)y_1;
    uint16_t val2 = (uint16_t)y_2;
    uint16_t val3 = (uint16_t)y_3;
    uint16_t val4 = (uint16_t)y_4;
    
    // Pack as little-endian bytes
    uint8_t data1[2] = {(uint8_t)(val1 & 0xFF), (uint8_t)((val1 >> 8) & 0xFF)};
    uint8_t data2[2] = {(uint8_t)(val2 & 0xFF), (uint8_t)((val2 >> 8) & 0xFF)};
    uint8_t data3[2] = {(uint8_t)(val3 & 0xFF), (uint8_t)((val3 >> 8) & 0xFF)};
    uint8_t data4[2] = {(uint8_t)(val4 & 0xFF), (uint8_t)((val4 >> 8) & 0xFF)};
    
    // Send via BLE
    pCharEmg1->setValue(data1, 2);
    pCharEmg1->notify();
    
    pCharEmg2->setValue(data2, 2);
    pCharEmg2->notify();
    
    pCharEmg3->setValue(data3, 2);
    pCharEmg3->notify();
    
    pCharEmg4->setValue(data4, 2);
    pCharEmg4->notify();
    
    // Print to serial for debugging
    if ((int)(counter * 100) % 100 == 0) { // Print every 1 second
      Serial.printf("Streaming - EMG: %.2f, y1: %.0f, y2: %.0f, y3: %.0f, y4: %.0f\n", 
                    counter, y_1, y_2, y_3, y_4);
    }
    
    counter += 0.01;
    delay(50);  // 20Hz update rate
  } else {
    delay(100);  // Wait when not connected
  }
}