#ifndef I2C_MCP23017_DRIVER_H
#define I2C_MCP23017_DRIVER_H

/**
 * @file i2c_mcp23017.h
 * @brief I2C driver for MCP23017 I/O expander.
 * @details
 * This driver provides an interface for communicating with the MCP23017 I/O expander
 * over I2C. It allows for reading and writing data to the MCP23017, as well as
 * configuring its registers for input/output operations.
 * 
 * @author Roberto Axt
 * @date 2025-08-20
 * @version 0.0
 * 
 * @par License 
 * This project is licensed under the MIT License. 
 */
 
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
 
// Máscaras de E/S pedidas
/**
 * @brief Example implementation.
 * @note Use this four defines to configure the inputs and outputs of each ports.
 */
#ifndef IODIRA_VALUE
#define IODIRA_VALUE 0x0F  // A0..A3 IN=1, A4..A7 OUT=0
#endif

#ifndef IODIRB_VALUE
#define IODIRB_VALUE 0xCF  // B0..B3 IN, B4..B5 OUT, B6..B7 IN
#endif

#ifndef GPIOA_OUT_MASK
#define GPIOA_OUT_MASK 0xF0 // A4..A7
#endif

#ifndef GPIOB_OUT_MASK
#define GPIOB_OUT_MASK 0x30 // B4..B5
#endif

/**
 * @brief Initializes the MCP23017 I/O expander over I2C.
 * @details This function sets up the MCP23017 by configuring its registers
 * for input/output operations. It also verifies communication with the device.
 * - IODIRA = 0x0F  (GPA0..3 IN, GPA4..7 OUT)
 * - IODIRB = 0xCF  (GPB0..3 IN, GPB4..5 OUT, GPB6..7 IN)
 * - GPPUA  = 0x0F  (Pull-ups on GPA0..3)
 * - GPPUB  = 0xCF  (Pull-ups on GPB0..3, GPB6..7)
 * - IOCON  = 0x20  (Bank=0, MIRROR=0, SEQOP=1, DISSLW=0, HAEN=1, ODR=0, INTPOL=0)
 * 
 * @param i2c_addr The I2C address of the MCP23017 (0x20 to 0x27).
 * @param timeout_ms The timeout for I2C operations in milliseconds.
 * @return ESP_OK on success, or an error code on failure.
 * @note This function should be called before any other MCP23017 operations.
 * @note It is expected that the I2C management driver has been initialized before calling this
 * function.
 */
esp_err_t i2c_mcp23017_start(uint8_t i2c_addr, int timeout_ms);

/**
 * @brief Configures the MCP23017 registers for setting up the internal pull-ups for the inputs pins.
 * @details This function configures the GPPUA and GPPUB registers of the MCP23017
 * to enable internal pull-up resistors on the specified GPIO pins.
 *
 * @param gppua_mask Bitmask for GPPUA register (0x0F for GPA0..3).
 * @param gppub_mask Bitmask for GPPUB register (0xCF for GPB0..3, GPB6..7).
 * @return ESP_OK on success, or an error code on failure.
 * @note This function should be called after i2c_mcp23017_start() to configure the pull-ups.
 * @note The gppua_mask and gppub_mask parameters allow selective enabling of pull-ups on specific GPIO pins.
 *       For example, to enable pull-ups on GPA0 and GPA1, set gppua_mask to 0x03.
 *       To enable pull-ups on GPB0 and GPB1, set gppub_mask to 0x03.
 */
esp_err_t i2c_mcp23017_set_pullups(uint8_t gppua_mask, uint8_t gppub_mask);

/**
 * @brief Writes outputs to the GPIOA port of the MCP23017.
 * @details This function writes a byte value to the GPIOA port of the MCP23017
 * to control the output state of the GPIO pins configured as outputs.
 * 
 * @param value The byte value to write to GPIOA (bits 0-3 for GPA0..3, bits 4-7 for GPA4..7).
 * @return ESP_OK on success, or an error code on failure.
 * @note This function should be called after configuring the GPIOA pins as outputs.
 * @note The value parameter allows setting the output state of the GPIOA pins.
 *       For example, to set GPA0 high and GPA1 low, set value to 0x01.
 *       To set GPA4 high and GPA5 low, set bits 4-7 accordingly.
 */
esp_err_t i2c_mcp23017_write_gpioa_outputs(uint8_t value);

/**
 * @brief Writes outputs to the GPIOB port of the MCP23017.
 * @details This function writes a byte value to the GPIOB port of the MCP23017
 * to control the output state of the GPIO pins configured as outputs.
 * @param value The byte value to write to GPIOB (bits 0-3 for GPB0..3, bits 4-5 for GPB4..5).
 * timeout.
 * @return ESP_OK on success, or an error code on failure.
 * @note This function should be called after configuring the GPIOB pins as outputs.
 * @note The value parameter allows setting the output state of the GPIOB pins.
 *      For example, to set GPB0 high and GPB1 low, set value
 * to 0x01.
 *     To set GPB4 high and GPB5 low, set bits 4-5 accordingly.
 * @note The GPB6 and GPB7 pins are always configured as inputs.
 *      Writing to these pins will not affect their state.
 */
esp_err_t i2c_mcp23017_write_gpiob_outputs(uint8_t value);

/**
 * @brief Reads inputs from the GPIOA port of the MCP23017.
 * @details This function reads a byte value from the GPIOA port of the MCP23017
 * to retrieve the input state of the GPIO pins configured as inputs.
 * 
 * @param value Pointer to a byte where the read value will be stored.
 * @return ESP_OK on success, or an error code on failure.
 * @note This function should be called after configuring the GPIOA pins as inputs.
 * @note The value parameter will contain the input state of the GPIOA pins after reading.
 *       For example, if GPA0 is high and GPA1 is low, value will be 0x01.
 */
esp_err_t i2c_mcp23017_read_gpioa_inputs(uint8_t *value);

/**
 * @brief Reads inputs from the GPIOB port of the MCP23017.
 * @details This function reads a byte value from the GPIOB port of the MCP23017
 * to retrieve the input state of the GPIO pins configured as inputs.
 * 
 * @param value Pointer to a byte where the read value will be stored.
 * @return ESP_OK on success, or an error code on failure.
 * @note This function should be called after configuring the GPIOB pins as inputs.
 * @note The value parameter will contain the input state of the GPIOB pins after reading.
 *       For example, if GPB0 is high and GPB1 is low, value will be 0x01.
 */
esp_err_t i2c_mcp23017_read_gpiob_inputs(uint8_t *value);

#endif // I2C_MCP23017_DRIVER_H 