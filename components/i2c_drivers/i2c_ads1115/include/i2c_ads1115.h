#ifndef I2C_ADS1115_DRIVER_H
#define I2C_ADS1115_DRIVER_H

/**
 * @file i2c_ads1115.h
 * @brief I2C driver for ADS1115 ADC.
 * @details
 * This driver provides an interface for communicating with the ADS1115 ADC
 * over I2C. It allows for reading analog values from the ADC channels and
 * configuring its registers for various operation modes.
 * 
 * @author Roberto Axt
 * @date 2025-08-21
 * @version 0.0
 * 
 * @par License 
 * This project is licensed under the MIT License.
 */

#include <stdint.h>
#include "esp_err.h"

/* Ganancia (PGA) -> rango de entrada y tamaño de LSB */
typedef enum {
    ADS1115_PGA_6V144 = 0, // ±6.144 V  (LSB 0.1875 mV) (no permitido si VDD < 5V, vea hoja de datos)
    ADS1115_PGA_4V096 = 1, // ±4.096 V  (LSB 0.125  mV)
    ADS1115_PGA_2V048 = 2, // ±2.048 V  (LSB 0.0625 mV) (valor por defecto recomendado)
    ADS1115_PGA_1V024 = 3, // ±1.024 V  (LSB 0.03125 mV)
    ADS1115_PGA_0V512 = 4, // ±0.512 V  (LSB 0.015625 mV)
    ADS1115_PGA_0V256 = 5  // ±0.256 V  (LSB 0.0078125 mV)
} ads1115_pga_t;

/* Data Rate (SPS) */
typedef enum {
    ADS1115_DR_8SPS   = 0,
    ADS1115_DR_16SPS  = 1,
    ADS1115_DR_32SPS  = 2,
    ADS1115_DR_64SPS  = 3,
    ADS1115_DR_128SPS = 4, // default típico
    ADS1115_DR_250SPS = 5,
    ADS1115_DR_475SPS = 6,
    ADS1115_DR_860SPS = 7
} ads1115_dr_t;

/* Channels */
typedef enum {
    ADS1115_CHANNEL_0 = 0,
    ADS1115_CHANNEL_1 = 1,
    ADS1115_CHANNEL_2 = 2,
    ADS1115_CHANNEL_3 = 3
} ads1115_channel_t; 

/**
  * @brief Initializes the ADS1115 ADC over I2C.
  * @details This function sets up the ADS1115 by configuring its registers
  * for operation. It also verifies communication with the device.
  * 
  * @param i2c_addr The I2C address of the ADS1115 (0x48 to 0x4B).
  * @param pga The programmable gain amplifier setting (see ads1115_pga_t).
  * @param dr The data rate setting (see ads1115_dr_t).
  * @param timeout_ms The timeout for I2C operations in milliseconds.
  * @return ESP_OK on success, or an error code on failure.
  * @note This function should be called before any other ADS1115 operations.
  * @note It is expected that the I2C management driver has been initialized before calling this
  * function.
  */
esp_err_t i2c_ads1115_start(uint8_t i2c_addr, ads1115_pga_t pga, ads1115_dr_t dr, int timeout_ms);

/**
 * @brief Reads a single-ended value from the specified ADS1115 channel.
 * @details This function initiates a single-ended conversion on the specified
 * channel and reads the resulting digital value from the ADS1115.
 * 
 * @param channel The ADC channel to read from (0 to 3).
 * @param value Pointer to an int16_t where the read value will be stored.
 * @return ESP_OK on success, or an error code on failure.
 * @note The input voltage range depends on the PGA setting configured during initialization.
 *       Ensure that the input voltage does not exceed the configured range to avoid damage.
 */
esp_err_t i2c_ads1115_read_single_ended(ads1115_channel_t channel, int16_t *value);



#endif // I2C_ADS1115_DRIVER_H