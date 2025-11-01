#ifndef I2C_PN532_DRIVER_H
#define I2C_PN532_DRIVER_H

/**
 * @file i2c_pn532.h
 * @brief I2C driver for PN532 NFC module.
 * @details
 * This driver provides an interface for communicating with the PN532 NFC module
 * over I2C. It allows for reading and writing data to the PN532, as well as
 * handling specific commands related to NFC operations.
 *
 * @author Roberto Axt
 * @date 2025-08-09
 * @version 0.0
 *
 * @par License
 * This project is licensed under the MIT License.
 */

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

/**
 * @brief Initializes the PN532 NFC module over I2C.
 * @details This function sets up the PN532 module by sending the SAMConfiguration command
 * and waiting for the appropriate acknowledgment. It also reads the response frame to ensure
 * that the module is ready for further operations.
 * 
 * @return ESP_OK on success, or an error code on failure.
 * @note This function should be called before any other PN532 operations.
 * @note It is expected that the I2C management driver has been initialized before calling this
 */
esp_err_t i2c_pn532_start(void);

/**
 * @brief Reads the UID of a passive NFC target.
 * @details This function sends the INLIST_PASSIVE_TARGET command to the PN532 module
 * and waits for a response containing the UID of a passive NFC target. The UID is stored
 * in the provided buffer, and its length is returned through the uid_len parameter.
 * 
 * @param uid Pointer to a buffer where the UID will be stored.
 * @param uid_len Pointer to a variable that will hold the length of the UID.
 * @return ESP_OK on success, or an error code on failure.
 * @note The uid buffer should be large enough to hold the expected UID length.
 * @note If no target is found, uid_len will be set to 0.
 * @note The function will block until a response is received or a timeout occurs.
 * @note The PN532 module must be initialized with i2c_pn532_start()
 * before calling this function.
 * @note The expected UID length is typically 4 to 10 bytes, depending on the NFC tag type.
 */
esp_err_t i2c_pn532_read_passive_target(uint8_t *uid, size_t *uid_len);

 #endif // I2C_PN532_DRIVER_H