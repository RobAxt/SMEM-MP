#include <stdio.h>

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "onewire_bus.h"
#include "ds18b20.h"

#include "ambiental_module.h"

#define ONEWIRE_BUS_GPIO    20
#define ONEWIRE_MAX_DS18B20 2
#define TEMPERATURE_READ_INTERVAL_MS 60000

static onewire_bus_handle_t bus = NULL;
static ds18b20_device_handle_t ds18b20s[ONEWIRE_MAX_DS18B20] = { NULL };
static uint8_t ds18b20_device_num = 0;

static hookCallback_onTemperatureRead hookCallback = NULL;
static ambiental_callback_data_t callback_data[ONEWIRE_MAX_DS18B20] = { 0 };

static const char *TAG = "ambiental_module";

static void temperatureRead_task(void *arg)
{
    while (1)
    {   
        esp_err_t err = ds18b20_trigger_temperature_conversion_for_all(bus);

        if (err == ESP_OK)
        {
            for (uint8_t i = 0; i < ds18b20_device_num; i++)
            {
                float temperature;
                if (ds18b20_get_temperature(ds18b20s[i], &temperature) == ESP_OK)
                {
                    // Prepare callback data
                    ds18b20_get_device_address(ds18b20s[i], (onewire_device_address_t *)&callback_data[i].sensor_id);
                    callback_data[i].temperature_celsius = temperature;

                    ESP_LOGI(TAG, "Sensor ID: %016llX, Temperature: %.2f °C",
                             callback_data[i].sensor_id,
                             callback_data[i].temperature_celsius);

                    // Invoke the registered callback
                    if (hookCallback != NULL)
                        hookCallback((void *)&callback_data[i], sizeof(ambiental_callback_data_t));
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to read temperature from DS18B20[%d]", i);
                }
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to trigger temperature conversion for all DS18B20 sensors");
        }

        vTaskDelay(pdMS_TO_TICKS(TEMPERATURE_READ_INTERVAL_MS));
    }
}

void ambiental_set_hookCallback_onTemperatureRead(hookCallback_onTemperatureRead cb)
{
    hookCallback = cb;
}

esp_err_t ambiental_module_start(void)
{
     // install 1-wire bus
    
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = ONEWIRE_BUS_GPIO,
        .flags = {
            .en_pull_up = true, // enable the internal pull-up resistor in case the external device didn't have one
        }
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10, // 1byte ROM command + 8byte ROM number + 1byte device command
    };
    
    if (onewire_new_bus_rmt(&bus_config, &rmt_config, &bus) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to install 1-Wire bus");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "1-Wire bus installed on GPIO%d", ONEWIRE_BUS_GPIO);
    
    // search for DS18B20 devices on the bus   
    
    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;
    esp_err_t search_result = ESP_OK;
    onewire_device_address_t address;

    // create 1-wire device iterator, which is used for device search
    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));
    ESP_LOGI(TAG, "Device iterator created, start searching...");

    while (search_result != ESP_ERR_NOT_FOUND) 
    {
        search_result = onewire_device_iter_get_next(iter, &next_onewire_device);
        if (search_result == ESP_OK) 
        { // found a new device, let's check if we can upgrade it to a DS18B20
            ds18b20_config_t ds_cfg = {};
            // check if the device is a DS18B20, if so, return the ds18b20 handle
            if (ds18b20_new_device_from_enumeration(&next_onewire_device, &ds_cfg, &ds18b20s[ds18b20_device_num]) == ESP_OK)
            {
                ds18b20_get_device_address(ds18b20s[ds18b20_device_num], &address);
                ESP_LOGI(TAG, "Found a DS18B20[%d], address: %016llX", ds18b20_device_num, address);
                ds18b20_device_num++;
                if (ds18b20_device_num >= ONEWIRE_MAX_DS18B20)
                {
                    ESP_LOGI(TAG, "Max DS18B20 number reached, stop searching...");
                    break;
                }
            } 
            else 
            {
                ESP_LOGI(TAG, "Found an unknown device, address: %016llX", next_onewire_device.address);
            }
        }
    } 

    ESP_ERROR_CHECK(onewire_del_device_iter(iter));
    ESP_LOGI(TAG, "Searching done, %d DS18B20 device(s) found", ds18b20_device_num);

    if(ds18b20_device_num == 0) 
    {
        ESP_LOGW(TAG, "No DS18B20 device found on the bus");
        return ESP_ERR_NOT_FOUND;
    }

    BaseType_t ok = xTaskCreate(temperatureRead_task, TAG, 2048, NULL, tskIDLE_PRIORITY + 1, NULL);

    if(ok != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create Read Temperature task");
        return ESP_FAIL;
    }

    return ESP_OK;   
}
