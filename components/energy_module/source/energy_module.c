#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "energy_module.h"

#define ENERGY_READ_INTERVAL_MS 60000

static hookCallback_onEnergyRead hookCallback = NULL;
static energy_data_t callback_data = {0};

static const char *TAG = "energy_module";

static void energyRead_task(void *arg) 
{
    while (1) 
    {
        
        vTaskDelay(pdMS_TO_TICKS(ENERGY_READ_INTERVAL_MS));
    }
}

void energy_set_hookCallback_onEnergyRead(hookCallback_onEnergyRead callback) 
{
    hookCallback = callback;
}

esp_err_t energy_module_start(void) 
{
    ESP_LOGI(TAG, "Energy module started");



 
    
    BaseType_t ok = xTaskCreate(energyRead_task, TAG, 2048, NULL, tskIDLE_PRIORITY + 1, NULL);

    if(ok != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create Read Energy task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
