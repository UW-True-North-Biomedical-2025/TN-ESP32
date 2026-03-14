/**************************************************************************************************/
/**
 * @file ble.cpp
 * @author  ()
 * @brief BLE transport implementation for the ESP32 telemetry bridge
 *
 * @version 0.1
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2026
 *
 */
/**************************************************************************************************/

/*------------------------------------------------------------------------------------------------*/
/* HEADERS                                                                                        */
/*------------------------------------------------------------------------------------------------*/

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "ble.h"

/*------------------------------------------------------------------------------------------------*/
/* MACROS                                                                                         */
/*------------------------------------------------------------------------------------------------*/

#define TN_BLE_DEVICE_NAME        "TN-XIAO-ESP32C3"
#define TN_BLE_SERVICE_UUID       "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define TN_BLE_TELEMETRY_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

namespace
{
    constexpr uint32_t k_ble_notify_period_ms = 50U;

    BLEServer *s_server = nullptr;
    BLECharacteristic *s_telemetry_characteristic = nullptr;
    bool s_device_connected = false;
    bool s_previous_device_connected = false;
    uint32_t s_last_notify_ms = 0U;

    class telemetry_server_callbacks_t : public BLEServerCallbacks
    {
    public:
        void onConnect(BLEServer *server) override
        {
            (void)server;
            s_device_connected = true;
            Serial.println("BLE client connected.");
        }

        void onDisconnect(BLEServer *server) override
        {
            (void)server;
            s_device_connected = false;
            Serial.println("BLE client disconnected.");
        }
    };

    String format_payload(const telemetry_sample_t &sample)
    {
        char payload[160] = {};
        snprintf(payload,
                 sizeof(payload),
                 "%lu,%.3f,%.3f,%.2f,%.2f,%.2f",
                 static_cast<unsigned long>(sample.timestamp_ms),
                 static_cast<double>(sample.emg_1),
                 static_cast<double>(sample.emg_2),
                 static_cast<double>(sample.imu_angle_deg),
                 static_cast<double>(sample.emg_fft_1_hz),
                 static_cast<double>(sample.emg_fft_2_hz));
        return String(payload);
    }
}

bool ble_init()
{
    BLEDevice::init(TN_BLE_DEVICE_NAME);

    s_server = BLEDevice::createServer();
    if (s_server == nullptr)
    {
        Serial.println("Failed to create BLE server.");
        return false;
    }

    s_server->setCallbacks(new telemetry_server_callbacks_t());

    BLEService *service = s_server->createService(TN_BLE_SERVICE_UUID);
    if (service == nullptr)
    {
        Serial.println("Failed to create BLE service.");
        return false;
    }

    s_telemetry_characteristic = service->createCharacteristic(
        TN_BLE_TELEMETRY_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    if (s_telemetry_characteristic == nullptr)
    {
        Serial.println("Failed to create BLE characteristic.");
        return false;
    }

    s_telemetry_characteristic->addDescriptor(new BLE2902());
    s_telemetry_characteristic->setValue("0,0.000,0.000,0.00,0.00,0.00");

    service->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(TN_BLE_SERVICE_UUID);
    advertising->setScanResponse(false);
    advertising->setMinPreferred(0x00);
    BLEDevice::startAdvertising();

    delay(3000);

    Serial.println("ESP32C3 BLE MAC Address: ");
    Serial.println(BLEDevice::getAddress().toString().c_str());

    Serial.println("BLE server ready and advertising.");
    return true;
}

void ble_update(const telemetry_sample_t &sample, uint32_t now_ms)
{
    if ((s_server == nullptr) || (s_telemetry_characteristic == nullptr))
    {
        return;
    }

    if (!s_device_connected && s_previous_device_connected)
    {
        delay(50);
        s_server->startAdvertising();
        Serial.println("BLE advertising restarted.");
        s_previous_device_connected = s_device_connected;
    }

    if (s_device_connected && !s_previous_device_connected)
    {
        s_previous_device_connected = s_device_connected;
        s_last_notify_ms = 0U;
    }

    if (!s_device_connected)
    {
        return;
    }

    if ((now_ms - s_last_notify_ms) < k_ble_notify_period_ms)
    {
        return;
    }

    s_last_notify_ms = now_ms;

    const String payload = format_payload(sample);
    s_telemetry_characteristic->setValue(
        reinterpret_cast<uint8_t *>(const_cast<char *>(payload.c_str())),
        payload.length());
    s_telemetry_characteristic->notify();
}

bool ble_is_client_connected()
{
    return s_device_connected;
}
