#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "energy_module.h"
#include "uart_pzem004t.h"

#define ENERGY_READ_INTERVAL_MS 60000

static hookCallback_onEnergyRead hookCallback = NULL;
static energy_data_t callback_data = {0};

static const char *TAG = "energy_module";

static void energyRead_task(void *arg) 
{
    while (1) 
    {
        esp_err_t err = uart_pzem004t_read();
        if(err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Failed to read from PZEM004T: %s", esp_err_to_name(err));
        }
        else
        {
            callback_data.ac_voltage = uart_pzem_voltage_V();
            callback_data.ac_current = uart_pzem_current_A();
            callback_data.ac_power = uart_pzem_power_W();
            callback_data.ac_frequency = uart_pzem_freq_Hz();
            callback_data.ac_power_factor = uart_pzem_pf();
        }

        
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

    esp_err_t err = uart_pzem004t_start(UART_NUM_1, GPIO_NUM_18, GPIO_NUM_19);
    if(err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start PZEM004T: %s", esp_err_to_name(err));
    }
    
    BaseType_t ok = xTaskCreate(energyRead_task, TAG, 2048, NULL, tskIDLE_PRIORITY + 1, NULL);

    if(ok != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create Read Energy task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
