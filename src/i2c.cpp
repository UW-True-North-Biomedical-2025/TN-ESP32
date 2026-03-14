/**************************************************************************************************/
/**
 * @file i2c.cpp
 * @author  ()
 * @brief ESP32/I2C/Arduino slave receiver for Teensy 4.0 telemetry packets
 *
 *   Receives float arrays from the Teensy master. Protocol:
 *   - Byte 0: number of floats N (1..k_i2c_max_floats)
 *   - Bytes 1..4*N: N IEEE 754 single-precision floats, little-endian (4 bytes each)
 *
 *   Current payload layout:
 *   - [0] EMG RMS 1
 *   - [1] EMG RMS 2
 *   - [2] IMU angle
 *   - [3] EMG FFT median frequency 1
 *   - [4] EMG FFT median frequency 2
 *
 *   XIAO ESP32C3 connections:
 *   - SDA: GPIO 6
 *   - SCL: GPIO 7
 *   - Common GND with Teensy 4.0
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
#include <Wire.h>
#include <string.h>
#include "i2c.h"

/*------------------------------------------------------------------------------------------------*/
/* GLOBAL VARIABLES                                                                               */
/*------------------------------------------------------------------------------------------------*/

namespace
{
    constexpr uint32_t k_i2c_bus_clock_hz = 100000U;
    constexpr uint32_t k_i2c_data_timeout_ms = 500U;
    constexpr size_t k_i2c_buffer_size_bytes = 64U;

    constexpr uint8_t k_payload_index_emg_1 = 0U;
    constexpr uint8_t k_payload_index_emg_2 = 1U;
    constexpr uint8_t k_payload_index_imu_angle = 2U;
    constexpr uint8_t k_payload_index_emg_fft_1 = 3U;
    constexpr uint8_t k_payload_index_emg_fft_2 = 4U;

    portMUX_TYPE s_i2c_mux = portMUX_INITIALIZER_UNLOCKED;
    float s_float_buffer[k_i2c_max_floats] = {};
    uint8_t s_float_count = 0U;
    bool s_new_packet = false;
    bool s_has_valid_packet = false;
    uint32_t s_last_packet_ms = 0U;
    uint8_t s_i2c_address = k_i2c_slave_address;

    void drain_receive_buffer()
    {
        while (Wire.available() > 0)
        {
            (void)Wire.read();
        }
    }

    void build_live_sample(const float *values,
                           uint8_t count,
                           telemetry_sample_t *sample,
                           uint32_t now_ms)
    {
        if ((values == nullptr) || (sample == nullptr))
        {
            return;
        }

        sample->timestamp_ms = now_ms;
        sample->emg_1 = (count > k_payload_index_emg_1) ? values[k_payload_index_emg_1] : 0.0f;
        sample->emg_2 = (count > k_payload_index_emg_2) ? values[k_payload_index_emg_2] : 0.0f;
        sample->imu_angle_deg =
            (count > k_payload_index_imu_angle) ? values[k_payload_index_imu_angle] : 0.0f;
        sample->emg_fft_1_hz = (count > k_payload_index_emg_fft_1) ? values[k_payload_index_emg_fft_1] : 0.0f;
        sample->emg_fft_2_hz = (count > k_payload_index_emg_fft_2) ? values[k_payload_index_emg_fft_2] : 0.0f;
        sample->uses_dummy_data = false;
    }

    void receive_event(int bytes)
    {
        if (bytes <= 0)
        {
            return;
        }

        if (Wire.available() <= 0)
        {
            return;
        }

        const int count_value = Wire.read();
        if ((count_value <= 0) || (count_value > static_cast<int>(k_i2c_max_floats)))
        {
            drain_receive_buffer();
            return;
        }

        const uint8_t count = static_cast<uint8_t>(count_value);
        const int expected_bytes = 1 + (static_cast<int>(count) * static_cast<int>(sizeof(float)));
        if (bytes < expected_bytes)
        {
            drain_receive_buffer();
            return;
        }

        float parsed_values[k_i2c_max_floats] = {};

        for (uint8_t value_index = 0; value_index < count; ++value_index)
        {
            uint8_t raw_bytes[sizeof(float)] = {};

            for (size_t byte_index = 0; byte_index < sizeof(float); ++byte_index)
            {
                if (Wire.available() <= 0)
                {
                    drain_receive_buffer();
                    return;
                }

                raw_bytes[byte_index] = static_cast<uint8_t>(Wire.read());
            }

            memcpy(&parsed_values[value_index], raw_bytes, sizeof(float));
        }

        drain_receive_buffer();

        const uint32_t packet_timestamp_ms = millis();

        portENTER_CRITICAL_ISR(&s_i2c_mux);
        memcpy(s_float_buffer, parsed_values, sizeof(parsed_values));
        s_float_count = count;
        s_new_packet = true;
        s_has_valid_packet = true;
        s_last_packet_ms = packet_timestamp_ms;
        portEXIT_CRITICAL_ISR(&s_i2c_mux);
    }

    void request_event()
    {
        float local_values[k_i2c_max_floats] = {};
        uint8_t local_count = 0U;

        portENTER_CRITICAL(&s_i2c_mux);
        memcpy(local_values, s_float_buffer, sizeof(local_values));
        local_count = s_float_count;
        portEXIT_CRITICAL(&s_i2c_mux);

        Wire.write(local_count);

        for (uint8_t value_index = 0; value_index < local_count; ++value_index)
        {
            const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&local_values[value_index]);
            Wire.write(bytes, sizeof(float));
        }
    }
}

bool i2c_init(uint8_t addr)
{
    s_i2c_address = addr;

    if (Wire.setBufferSize(k_i2c_buffer_size_bytes) == 0U)
    {
        Serial.println("I2C buffer resize failed.");
    }

    const bool started = Wire.begin(
        s_i2c_address,
        static_cast<int>(k_xiao_esp32c3_sda_pin),
        static_cast<int>(k_xiao_esp32c3_scl_pin),
        k_i2c_bus_clock_hz);

    if (!started)
    {
        return false;
    }

    Wire.onReceive(receive_event);
    Wire.onRequest(request_event);

    Serial.print("I2C slave ready at address 0x");
    Serial.print(s_i2c_address, HEX);
    Serial.print(" on SDA=");
    Serial.print(k_xiao_esp32c3_sda_pin);
    Serial.print(" SCL=");
    Serial.print(k_xiao_esp32c3_scl_pin);
    Serial.print(" @ ");
    Serial.print(k_i2c_bus_clock_hz);
    Serial.println(" Hz");

    return true;
}

bool i2c_has_new_packet()
{
    bool has_new_packet = false;

    portENTER_CRITICAL(&s_i2c_mux);
    has_new_packet = s_new_packet;
    portEXIT_CRITICAL(&s_i2c_mux);

    return has_new_packet;
}

uint8_t i2c_get_last_float_array(float *out)
{
    float local_values[k_i2c_max_floats] = {};
    uint8_t local_count = 0U;

    portENTER_CRITICAL(&s_i2c_mux);
    memcpy(local_values, s_float_buffer, sizeof(local_values));
    local_count = s_float_count;
    s_new_packet = false;
    portEXIT_CRITICAL(&s_i2c_mux);

    if (out != nullptr)
    {
        memcpy(out, local_values, sizeof(local_values));
    }

    return local_count;
}

bool i2c_has_live_data(uint32_t now_ms)
{
    bool has_valid_packet = false;
    uint32_t last_packet_ms = 0U;

    portENTER_CRITICAL(&s_i2c_mux);
    has_valid_packet = s_has_valid_packet;
    last_packet_ms = s_last_packet_ms;
    portEXIT_CRITICAL(&s_i2c_mux);

    return has_valid_packet && ((now_ms - last_packet_ms) <= k_i2c_data_timeout_ms);
}

bool i2c_get_latest_sample(telemetry_sample_t *sample, uint32_t now_ms)
{
    if (sample == nullptr)
    {
        return false;
    }

    *sample = telemetry_sample_t {};

    float local_values[k_i2c_max_floats] = {};
    uint8_t local_count = 0U;
    bool has_valid_packet = false;
    uint32_t last_packet_ms = 0U;

    portENTER_CRITICAL(&s_i2c_mux);
    memcpy(local_values, s_float_buffer, sizeof(local_values));
    local_count = s_float_count;
    has_valid_packet = s_has_valid_packet;
    last_packet_ms = s_last_packet_ms;
    portEXIT_CRITICAL(&s_i2c_mux);

    if (has_valid_packet && ((now_ms - last_packet_ms) <= k_i2c_data_timeout_ms))
    {
        build_live_sample(local_values, local_count, sample, last_packet_ms);
        return true;
    }

    return false;
}
