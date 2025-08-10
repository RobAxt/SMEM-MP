#ifndef I2C_MGMT_DRIVER_H
#define I2C_MGMT_DRIVER_H

/**
 * @file i2c_mgmt_driver.h
 * @brief I2C management driver for ESP-IDF.
 * @details
 * This driver provides a simple interface for managing I2C communication
 * using FreeRTOS queues. It allows for asynchronous I2C operations, enabling
 * multiple I2C requests to be queued and processed in a separate task.
 * 
 * @author  Roberto Axt
 * @date    2025-08-09
 * @version 0.0
 * 
 * @par License
 * This project is licensed under the MIT License.
 */

#include <stdint.h> // For uint8_t, uint16_t, etc.  
#include <stdbool.h> // For bool type

#include "driver/i2c.h"
#include "freertos/queue.h"

typedef enum { I2C_OP_READ=0, I2C_OP_WRITE } i2c_operation_t;

typedef struct {
    i2c_operation_t op;
    uint8_t device_addr;
    const uint8_t *tx_buffer;
    size_t tx_len;
    uint8_t *rx_buffer;
    size_t rx_len;
    TickType_t timeout_ticks;
    QueueHandle_t response_queue;
} i2c_request_t;

/**
 * @brief Inicializes the I2C management driver.
 * @param i2c_port The I2C port to use (e.g., I2C_NUM_0).
 * @param sda_pin The GPIO pin for SDA.
 * @param scl_pin The GPIO pin for SCL.
 * @param clk_speed The clock speed for I2C communication in Hz.
 * @param queue_size The size of the queue for I2C requests.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t i2c_mgmt_start(i2c_port_t i2c_port, gpio_num_t sda_pin, gpio_num_t scl_pin, size_t queue_size);

/**
 * @brief returns the queue handle for I2C requests.
 * @return The queue handle for I2C requests.
 */
QueueHandle_t i2c_mgmt_get_queue(void);

#endif // I2C_MGMT_DRIVER_H