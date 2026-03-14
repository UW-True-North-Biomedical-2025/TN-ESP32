/**************************************************************************************************/
/**
 * @file main.cpp
 * @author  ()
 * @brief Main application entry point for the ESP32 I2C-to-BLE bridge
 *
 * @version 0.1
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2026
 *
 */
/**************************************************************************************************/

#include <Arduino.h>
#include "ble.h"
#include "i2c.h"

namespace
{
    constexpr uint32_t k_serial_baud_rate = 115200U;
    constexpr uint32_t k_status_print_period_ms = 1000U;

    void print_i2c_packet_summary(const float *values, uint8_t count)
    {
        Serial.print("I2C packet [");
        Serial.print(count);
        Serial.print("]: ");

        for (uint8_t index = 0; index < count; ++index)
        {
            if (index != 0U)
            {
                Serial.print(", ");
            }

            Serial.print(values[index], 4);
        }

        Serial.println();
    }

    void print_bridge_status(const telemetry_sample_t &sample)
    {
        Serial.print("Bridge mode=I2C");
        Serial.print(" ble=");
        Serial.print(ble_is_client_connected() ? "connected" : "idle");
        Serial.print(" payload=");
        Serial.print(sample.timestamp_ms);
        Serial.print(",");
        Serial.print(sample.emg_1, 3);
        Serial.print(",");
        Serial.print(sample.emg_2, 3);
        Serial.print(",");
        Serial.print(sample.imu_angle_deg, 2);
        Serial.print(",");
        Serial.print(sample.emg_fft_1_hz, 2);
        Serial.print(",");
        Serial.println(sample.emg_fft_2_hz, 2);
    }

    void print_no_i2c_status()
    {
        Serial.print("Bridge mode=IDLE ble=");
        Serial.print(ble_is_client_connected() ? "connected" : "idle");
        Serial.println(" no i2c data incoming.");
    }
}

void setup()
{
    Serial.begin(k_serial_baud_rate);

    const unsigned long serial_wait_start = millis();
    while (!Serial && ((millis() - serial_wait_start) < 2000U))
    {
        delay(10);
    }

    Serial.println("Starting TN ESP32 I2C-to-BLE bridge.");

    if (!i2c_init())
    {
        Serial.println("I2C initialization failed.");
    }

    if (!ble_init())
    {
        Serial.println("BLE initialization failed.");
    }
}

void loop()
{
    static uint32_t last_status_print_ms = 0U;

    const uint32_t now_ms = millis();

    if (i2c_has_new_packet())
    {
        float raw_packet[k_i2c_max_floats] = {};
        const uint8_t count = i2c_get_last_float_array(raw_packet);
        print_i2c_packet_summary(raw_packet, count);
    }

    telemetry_sample_t sample = {};
    const bool has_live_i2c = i2c_get_latest_sample(&sample, now_ms);

    if (has_live_i2c)
    {
        ble_update(sample, now_ms);
    }

    if ((now_ms - last_status_print_ms) >= k_status_print_period_ms)
    {
        last_status_print_ms = now_ms;
        if (has_live_i2c)
        {
            print_bridge_status(sample);
        }
        else
        {
            print_no_i2c_status();
        }
    }

    delay(10);
}
