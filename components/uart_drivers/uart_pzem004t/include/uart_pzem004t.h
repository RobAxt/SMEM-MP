#ifndef UART_PZEM004T_H
#define UART_PZEM004T_H

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_err.h"

/**
 * @file uart_pzem004t.h
 * @brief Header file for UART PZEM004T driver.
 * @details This file contains function declarations and macros for 
 *          interfacing with the PZEM004T energy monitor via UART.
 * 
 * @author Roberto Axt
 * @date 2025-11-02
 * 
 * @par License
 * This file is part of the SMEM-MP project and is licensed under the MIT License.
 */

/**
 * @brief Initializes the UART PZEM004T driver.
 * @param uart_num The UART port number to use.
 * @param tx_io_num The GPIO number for UART TX.
 * @param rx_io_num The GPIO number for UART RX.
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
 esp_err_t uart_pzem004t_start(uart_port_t uart_num, gpio_num_t tx_io_num, gpio_num_t rx_io_num);

/**
 * @brief Reads data from the PZEM004T device.
 * @param None
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
 esp_err_t uart_pzem004t_read(void);
 
/**
 * @brief Resets the PZEM004T device.
 * @param None
 * @return esp_err_t Returns ESP_OK on success, or an error code on failure.
 */
 esp_err_t uart_pzem_reset(void);
 
#endif // UART_PZEM004T_H
