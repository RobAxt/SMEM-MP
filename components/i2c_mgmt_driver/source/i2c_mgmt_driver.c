#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "i2c_mgmt_driver.h"

static const char *TAG = "i2c_mgmt";

static QueueHandle_t i2c_queue = NULL;
static i2c_port_t active_i2c_port;

static void i2c_mgmt_task(void *param);

esp_err_t i2c_mgmt_start(i2c_port_t i2c_port, gpio_num_t sda_pin, gpio_num_t scl_pin, size_t queue_size)
{
    active_i2c_port = i2c_port;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 100000, // 100kHz
    };

    esp_err_t err;
    err = i2c_param_config(i2c_port, &conf);
    if (err != ESP_OK) return err;

    err = i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK) return err;

    i2c_queue = xQueueCreate(queue_size, sizeof(i2c_request_t));
    if (i2c_queue == NULL) return ESP_ERR_NO_MEM;

    configASSERT(pdPASS == xTaskCreate(i2c_mgmt_task, "i2c_mgmt_task", 4096, NULL, 10, NULL));
    
    ESP_LOGI(TAG, "Dispatcher I2C inicializado en puerto %d", i2c_port);
    return ESP_OK;
}

QueueHandle_t i2c_mgmt_get_queue(void)
{
    return i2c_queue;
}

/**
 * @brief Task that handles I2C requests from the queue.
 * This task processes both read and write operations
 * and sends the result back to the requesting task.   
 * @param param Unused parameter.
 * @return void
 */

static void i2c_mgmt_task(void *param)
{
    i2c_request_t req;
    while (1)
    {
        if (xQueueReceive(i2c_queue, &req, portMAX_DELAY))
        {
            esp_err_t ret = ESP_FAIL;

            if(req.device_addr < 0x08 || req.device_addr > 0x77) {
                ESP_LOGE(TAG, "Dirección I2C inválida: %02X: %s", req.device_addr, esp_err_to_name(ret));
                if (req.response_queue != NULL) {
                    xQueueSend(req.response_queue, &ret, portMAX_DELAY);
                }
                continue;
            }

            if (req.op == I2C_OP_WRITE)
            {
                ret = i2c_master_write_to_device(
                    active_i2c_port, req.device_addr,
                    req.tx_buffer, req.tx_len,
                    req.timeout_ticks
                );

                if(ESP_OK != ret) 
                    ESP_LOGE(TAG, "Failed to write to device %02X: %s ", req.device_addr, esp_err_to_name(ret));
            } 
            
            if (req.op == I2C_OP_READ)
            {
                ret = i2c_master_read_from_device(
                    active_i2c_port, req.device_addr,
                    req.rx_buffer, req.rx_len,
                    req.timeout_ticks
                );

                if(ESP_OK != ret) 
                    ESP_LOGE(TAG, "Failed to read from device %02X: %s", req.device_addr, esp_err_to_name(ret));
            }

            if (req.response_queue != NULL) 
                xQueueSend(req.response_queue, &ret, portMAX_DELAY);
        }
    }
}