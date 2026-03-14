/**************************************************************************************************/
/**
 * @file telemetry.h
 * @author  ()
 * @brief Shared telemetry model for the ESP32 I2C-to-BLE bridge
 *
 * @version 0.1
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2026
 *
 */
/**************************************************************************************************/

#ifndef TELEMETRY_H
#define TELEMETRY_H

/*------------------------------------------------------------------------------------------------*/
/* HEADERS                                                                                        */
/*------------------------------------------------------------------------------------------------*/

#include <Arduino.h>

/*------------------------------------------------------------------------------------------------*/
/* CLASS DECLARATIONS                                                                             */
/*------------------------------------------------------------------------------------------------*/

/**************************************************************************************************/
/**
 * @brief Telemetry sample exposed to the BLE transport.
 *
 * The outgoing CSV payload format is:
 * `timestamp_ms,emg_1,emg_2,imu_angle_deg,emg_fft_1_hz,emg_fft_2_hz`
 */
/**************************************************************************************************/
struct telemetry_sample_t
{
    uint32_t timestamp_ms = 0U;
    float emg_1 = 0.0f;
    float emg_2 = 0.0f;
    float imu_angle_deg = 0.0f;
    float emg_fft_1_hz = 0.0f;
    float emg_fft_2_hz = 0.0f;
    bool uses_dummy_data = true;
};

#endif // TELEMETRY_H
