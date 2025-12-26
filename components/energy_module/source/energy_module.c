#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "energy_module.h"
#include "uart_pzem004t.h"
#include "i2c_mgmt_driver.h"
#include "i2c_ads1115.h"
#include "zigbee_gateway.h"

#define ENERGY_READ_INTERVAL_MS 60000
#define ENERGY_STATE_INTERVAL_MS 2000

static hookCallback_onEnergyEvent hookEnergyReadCallback = NULL;
static hookCallback_onEnergyEvent hookEnergyStateCallback = NULL;
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

            ESP_LOGI(TAG, "AC Voltage=%.1f V, AC Current=%.3f A, AC Power=%.1f W, AC freq=%.1f Hz, AC PF=%.2f",
             callback_data.ac_voltage, callback_data.ac_current, callback_data.ac_power, 
             callback_data.ac_frequency, callback_data.ac_power_factor);
        }

        int16_t temp_value = 0;

        err = i2c_ads1115_read_single_ended(ADS1115_CHANNEL_0, &temp_value);

        if(err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Failed to read from ADS1115= %s", esp_err_to_name(err));
        }
        else
        {
            callback_data.dc_voltage = ((float) temp_value)/100.0;
            temp_value = 0;
            ESP_LOGI(TAG, "DC Voltage: %.1f",callback_data.dc_voltage);
        }

        err = i2c_ads1115_read_single_ended(ADS1115_CHANNEL_1, &temp_value);

        if(err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Failed to read from ADS1115= %s V", esp_err_to_name(err));
        }
        else
        {
            callback_data.dc_current = ((float) temp_value)/100.0;
            callback_data.dc_power = callback_data.dc_current * callback_data.dc_voltage;

            ESP_LOGI(TAG, "DC Current= %.1f A, DC Power= %.1f ",callback_data.dc_current, callback_data.dc_power );
        }

        if(hookEnergyReadCallback != NULL)
            hookEnergyReadCallback(&callback_data);
        
        vTaskDelay(pdMS_TO_TICKS(ENERGY_READ_INTERVAL_MS));
    }
}

static void energyState_task(void *arg) 
{
    uint8_t zb_state = 0;
    uint8_t last_state = 0xFF;

    while (1) 
    {
        esp_err_t err = zigbee_gateway_data_receive(&zb_state, sizeof(zb_state));

        if(hookEnergyStateCallback != NULL && err == ESP_OK) 
        {
            if( zb_state != last_state)
            {
                callback_data.zigbee_device_state = zb_state;
                hookEnergyStateCallback(&callback_data);
                last_state = zb_state;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(ENERGY_STATE_INTERVAL_MS));
    }
}

void energy_set_hookCallback_onEnergyRead(hookCallback_onEnergyEvent callback) 
{
    hookEnergyReadCallback = callback;
}

void energy_set_hookCallback_onEnergyState(hookCallback_onEnergyEvent callback) 
{
    hookEnergyStateCallback = callback;
}

esp_err_t energy_module_start(void) 
{
    ESP_LOGI(TAG, "Energy module started");

    esp_err_t err_pzem = uart_pzem004t_start(UART_NUM_1, GPIO_NUM_18, GPIO_NUM_19);
    if(err_pzem != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start PZEM004T: %s", esp_err_to_name(err_pzem));
    }
    
    esp_err_t err_i2c = i2c_mgmt_start(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22);
    if(err_i2c != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start I2C Manager: %s", esp_err_to_name(err_i2c));
    }
    else
    {
        err_i2c = i2c_ads1115_start(0x48, ADS1115_PGA_2V048, ADS1115_DR_128SPS, 100);
        if(err_i2c != ESP_OK) 
        {
            ESP_LOGE(TAG, "Failed to start ADS1115: %s", esp_err_to_name(err_i2c));
        }
    }

    esp_err_t err_zb = zigbee_gateway_start();
    if(err_zb != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start Zigbee Gateway: %s", esp_err_to_name(err_zb));
    }

    if( err_i2c == ESP_OK && err_pzem == ESP_OK && err_zb == ESP_OK )
    {
        BaseType_t ok = xTaskCreate(energyRead_task, TAG, 6144, NULL, tskIDLE_PRIORITY + 1, NULL);

        if(ok != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to create Read Energy task");
            return ESP_FAIL;
        }

        ok = xTaskCreate(energyState_task, TAG, 4096, NULL, tskIDLE_PRIORITY + 1, NULL);

        if(ok != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to create Energy State task");
            return ESP_FAIL;
        }
        return ESP_OK;
    }
    ESP_LOGE(TAG, "Failed to create start Energy module");
    return ESP_FAIL;
}
