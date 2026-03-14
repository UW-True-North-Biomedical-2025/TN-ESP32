/**************************************************************************************************/
/**
 * @file i2c.h
 * @author  ()
 * @brief ESP32 I2C slave interface for telemetry packets from the Teensy 4.0 master
 *
 * @version 0.1
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2026
 *
 */
/**************************************************************************************************/

#ifndef I2C_H
#define I2C_H

/*------------------------------------------------------------------------------------------------*/
// HEADERS                                                                                        */
/*------------------------------------------------------------------------------------------------*/

#include <Arduino.h>
#include <stdint.h>
#include "telemetry.h"

/*------------------------------------------------------------------------------------------------*/
// GLOBAL VARIABLES                                                                               */
/*------------------------------------------------------------------------------------------------*/

constexpr uint8_t k_i2c_slave_address = 0x08;
constexpr uint8_t k_i2c_max_floats = 5;
constexpr uint8_t k_xiao_esp32c3_sda_pin = 6;
constexpr uint8_t k_xiao_esp32c3_scl_pin = 7;

/*------------------------------------------------------------------------------------------------*/
// CLASS DECLARATIONS                                                                             */
/*------------------------------------------------------------------------------------------------*/



/*------------------------------------------------------------------------------------------------*/
// FUNCTION DECLARATIONS                                                                          */
/*------------------------------------------------------------------------------------------------*/

/**************************************************************************************************/
/**
 * @name i2c_init
 * @brief Initialize the I2C interface
 *
 *
 * @param addr
 *
 * @return true
 * @return false
 */
/**************************************************************************************************/
bool i2c_init(uint8_t addr = k_i2c_slave_address);

/**************************************************************************************************/
/**
 * @name i2c_has_new_packet
 * @brief Check if a new float-array packet has been received since the last raw-buffer read.
 *
 * @return true
 * @return false
 */
/**************************************************************************************************/
bool i2c_has_new_packet();

/**************************************************************************************************/
/**
 * @name i2c_get_last_float_array
 * @brief Copy the last received float array into `out` (must hold at least `k_i2c_max_floats`).
 *        Returns the number of floats received.
 *        Clears the "new packet" flag.
 *
 * @param out
 *
 * @return uint8_t
 */
/**************************************************************************************************/
uint8_t i2c_get_last_float_array(float *out);

/**************************************************************************************************/
/**
 * @name i2c_has_live_data
 * @brief Report whether the most recent Teensy packet is still considered fresh.
 *
 * @param now_ms
 *
 * @return true
 * @return false
 */
/**************************************************************************************************/
bool i2c_has_live_data(uint32_t now_ms);

/**************************************************************************************************/
/**
 * @name i2c_get_latest_sample
 * @brief Convert the most recent fresh I2C payload into the BLE telemetry model.
 *        When no fresh I2C payload is available, the output sample is cleared and no fallback data
 *        is generated.
 *
 * @param sample
 * @param now_ms
 *
 * @return true Returns true when the sample came from fresh I2C data.
 * @return false Returns false when no fresh I2C payload is available.
 */
/**************************************************************************************************/
bool i2c_get_latest_sample(telemetry_sample_t *sample, uint32_t now_ms);

#endif // I2C_H
