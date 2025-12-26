#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "zigbee_gateway.h"

static const char *TAG = "app_main";


void app_main(void)
{
    // Inicializar NVS (Non-Volatile Storage) para configuración persistente
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting Zigbee Gateway...");
    ret = zigbee_gateway_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Zigbee Gateway: %d", ret);
        return;
    }

    // Datos recibidos del nodo remoto
    uint8_t data = 0x00;

    while(1) {
        zigbee_gateway_data_receive(&data, sizeof(data));
        ESP_LOGI(TAG, "Received data from Zigbee device: 0x%02X", data);
        vTaskDelay(pdMS_TO_TICKS(7000));
    }
}