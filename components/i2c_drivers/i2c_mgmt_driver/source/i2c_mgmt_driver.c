#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "i2c_mgmt_driver.h"

#define I2C_MGMT_DEFAULT_SPEED_HZ 100000  // 100 kHz por defecto
#define I2C_MGMT_USE_INTERNAL_PULLUPS 0   // 1 = usa pull-ups internos; 0 = solo externos

static const char *TAG = "i2c_mgmt";

// Estado global (un único bus administrado)
static i2c_master_bus_handle_t bus = NULL;
static SemaphoreHandle_t mutex = NULL;
static TaskHandle_t owner = NULL;
static bool initialized = false;

esp_err_t i2c_mgmt_start(i2c_port_t i2c_port, gpio_num_t sda_pin, gpio_num_t scl_pin)
{
    if (initialized)
    {
        ESP_LOGW(TAG, "I2C bus already initialized");
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = i2c_port,                 // I2C_NUM_0 / I2C_NUM_1 según MCU
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,               // filtrado típico de glitches
        .flags = {
            .enable_internal_pullup = I2C_MGMT_USE_INTERNAL_PULLUPS ? 1 : 0,
        },
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        return err;
    }

    mutex = xSemaphoreCreateMutex();
    if (!mutex)
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        (void)i2c_del_master_bus(bus);
        bus = NULL;
        return ESP_ERR_NO_MEM;
    }

    owner = NULL;
    initialized = true;
    ESP_LOGI(TAG, "I2C bus initialized (port %d, SDA=%d, SCL=%d, %u Hz, pullups=%s)",
             (int)i2c_port, (int)sda_pin, (int)scl_pin,
             (unsigned)I2C_MGMT_DEFAULT_SPEED_HZ,
             I2C_MGMT_USE_INTERNAL_PULLUPS ? "internal" : "external");
    return ESP_OK;
}

esp_err_t i2c_mgmt_begin_transaction(void)
{
    if (!initialized)
        return ESP_ERR_INVALID_STATE;

    if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE)
        return ESP_ERR_TIMEOUT;

    owner = xTaskGetCurrentTaskHandle();
    ESP_LOGI(TAG, "I2C bus locked by task: %s", pcTaskGetName(owner));

    return ESP_OK;
}

static inline bool owner_ok(void)
{
    return owner == xTaskGetCurrentTaskHandle();
}

static esp_err_t create_device(uint8_t dev_addr, i2c_master_dev_handle_t *out)
{
    i2c_master_dev_handle_t dev = NULL;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = dev_addr,
        .scl_speed_hz    = I2C_MGMT_DEFAULT_SPEED_HZ,
    };

    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &dev);

    if (err != ESP_OK) 
        return err;

    *out = dev;
    return ESP_OK;
}

static void destroy_device(i2c_master_dev_handle_t dev)
{
    if (dev)
        (void)i2c_master_bus_rm_device(dev);
}

esp_err_t i2c_mgmt_write(uint8_t device_addr, const uint8_t *tx_buffer, size_t tx_len, int timeout_ms)
{
    if (!initialized) 
        return ESP_ERR_INVALID_STATE;
    
    if (!owner_ok())
    {
        ESP_LOGE(TAG, "Write called outside of owned transaction");
        return ESP_ERR_INVALID_STATE;
    }

    if (!tx_buffer || tx_len == 0)
        return ESP_ERR_INVALID_ARG;

    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = create_device(device_addr, &dev);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "add_device(0x%02X) failed: %s", device_addr, esp_err_to_name(err));
        return err;
    }

    err = i2c_master_transmit(dev, tx_buffer, tx_len, timeout_ms);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "transmit to 0x%02X failed: %s", device_addr, esp_err_to_name(err));

    destroy_device(dev);
    return err;
}

esp_err_t i2c_mgmt_read(uint8_t device_addr, uint8_t *rx_buffer, size_t *rx_len, int timeout_ms)
{
    if (!initialized) 
        return ESP_ERR_INVALID_STATE;
    
    if (!owner_ok())
    {
        ESP_LOGE(TAG, "Read called outside of owned transaction");
        return ESP_ERR_INVALID_STATE;
    }

    if(!rx_buffer || !rx_len || *rx_len == 0)
        return ESP_ERR_INVALID_ARG;
    
    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = create_device(device_addr, &dev);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "add_device(0x%02X) failed: %s", device_addr, esp_err_to_name(err));
        return err;
    }

    size_t requested = *rx_len;
    err = i2c_master_receive(dev, rx_buffer, requested, timeout_ms);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "receive from 0x%02X failed: %s", device_addr, esp_err_to_name(err));
        destroy_device(dev);
        return err;
    }

    *rx_len = requested; // la API entrega exactamente lo solicitado si retorna OK
    destroy_device(dev);
    return ESP_OK;
}

esp_err_t i2c_mgmt_end_transaction(void)
{
    if (!initialized)
        return ESP_ERR_INVALID_STATE;

    if (!owner_ok())
    {
        ESP_LOGE(TAG, "End transaction called by non-owner task");
        return ESP_ERR_INVALID_STATE;
    }
    
    owner = NULL;
    xSemaphoreGive(mutex);
    ESP_LOGI(TAG, "I2C bus released by task: %s", pcTaskGetName(owner));
    
    return ESP_OK;
}