#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "i2c_mgmt_driver.h"
#include "i2c_pn532.h"
#include "i2c_mcp23017.h"
#include "i2c_ads1115.h"

static const char *TAG = "SMEM-MP";

void rtos_print_task_list(void)
{
    // vTaskList escribe una tabla con: Name  State  Prio  Stack  Num  Affinity
    // Requiere CONFIG_FREERTOS_USE_TRACE_FACILITY y CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
    const size_t buf_sz = 2048; // Ajuste si hay muchas tareas
    char *buf = (char *)pvPortMalloc(buf_sz);
    if (!buf) { ESP_LOGE(TAG, "No hay memoria para buffer de tareas"); return; }

    vTaskList(buf);
    ESP_LOGI(TAG, "\nTask          State    Prio     Stack Core\n%s", buf);
    vPortFree(buf);
}

void app_main(void)
{    
    //esp_log_level_set("i2c_pn532", ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "Initializing SMEM-MP application");
    ESP_ERROR_CHECK(i2c_mgmt_start(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22)); // SDA=21, SCL=22
    ESP_LOGI(TAG, "I2C management driver started");
    ESP_ERROR_CHECK(i2c_pn532_start());
    ESP_LOGI(TAG, "PN532 NFC module initialized");
    ESP_ERROR_CHECK(i2c_mcp23017_start(0x20, 100)); // Dirección I2C=0x20, timeout=1000ms
    ESP_LOGI(TAG, "MCP23017 I/O expander initialized");
    ESP_ERROR_CHECK(i2c_mcp23017_set_pullups(IODIRA_VALUE, IODIRB_VALUE));
    ESP_LOGI(TAG, "MCP23017 pull-ups configured: GPA=0x%02X, GPB=0x%02X", IODIRA_VALUE, IODIRB_VALUE);
    ESP_ERROR_CHECK(i2c_ads1115_start(0x48, ADS1115_PGA_2V048, ADS1115_DR_128SPS, 100));
    ESP_LOGI(TAG, "ADS1115 ADC initialized");

    uint8_t uid[10] = {0};
    size_t uid_len = sizeof(uid);
    
    uint8_t gpioa_out = 0;
    gpioa_out |= (1<<4); // set high GPA4
    uint8_t gpiob_out = 0; 
    gpiob_out |= (1<<4); // set high GPB4

    while(1)
    {
        ESP_LOGI(TAG, "Running main loop");
        
        uid_len = sizeof(uid);
        esp_err_t err = i2c_pn532_read_passive_target(uid, &uid_len);
        
        if (err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Failed to read passive target: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(5000)); // Wait before retrying
            continue;
        }
        
        if(uid_len > 0)
            ESP_LOGI(TAG, "Read passive target (%d) UID: %02X, %02X, %02X, %02X", (int)uid_len, uid[0], uid[1], uid[2], uid[3]);
        else
            ESP_LOGW(TAG, "No UID read, check if a tag is present");

        vTaskDelay(pdMS_TO_TICKS(2500));

        // -------------------------------------------------------------------------------------------------------------------

        // Leer entradas del MCP23017 puerto A
        // A0..3 son entradas, A4..7 son salidas
        uint8_t gpioa = 0;
        err = i2c_mcp23017_read_gpioa_inputs(&gpioa);
        if (err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Failed to read GPIOA inputs: %s", esp_err_to_name(err));
        } 
        else 
        {
            ESP_LOGI(TAG, "GPIOA inputs: 0x%02X (A0..3: %d, %d, %d, %d)",
                     gpioa, (gpioa & 0x01) ? 1 : 0, (gpioa & 0x02) ? 1 : 0, (gpioa & 0x04) ? 1 : 0, (gpioa & 0x08) ? 1 : 0);
        }        
        
        // Leer entradas del MCP23017 puerto B
        // B0..3 son entradas, B4..5 son salidas, B6..7 son entradas
        uint8_t gpiob = 0;
        err = i2c_mcp23017_read_gpiob_inputs(&gpiob);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read GPIOB inputs: %s", esp_err_to_name(err));
        } 
        else
        {
            ESP_LOGI(TAG, "GPIOB inputs: 0x%02X (B0..3: %d, %d, %d, %d; B6..7: %d, %d)",
                     gpiob, (gpiob & 0x01) ? 1 : 0, (gpiob & 0x02) ? 1 : 0, (gpiob & 0x04) ? 1 : 0, (gpiob & 0x08) ? 1 : 0,
                            (gpiob & 0x40) ? 1 : 0, (gpiob & 0x80) ? 1 : 0);
        }

        //Clear GPA4 on every call // REMEMBER --> set |= (1<<4) - clear &= ~(1<<4) - toggle ^= (1<<4)
        gpioa_out ^= (1 << 4); // GPA4
        err = i2c_mcp23017_write_gpioa_outputs(gpioa_out);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to write GPIOA outputs: %s", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Wrote GPIOA outputs: 0x%02X", gpioa_out);
        }

        //Toogle GPB4 on every call REMEMBER --> set |= (1<<4) - clear &= ~(1<<4) - toggle ^= (1<<4)
        gpiob_out ^= (1 << 4);   // GPB4   
        err = i2c_mcp23017_write_gpiob_outputs(gpiob_out);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to write GPIOB outputs: %s", esp_err_to_name(err));
        } 
        else 
        {    
            ESP_LOGI(TAG, "Wrote GPIOB outputs: 0x%02X", gpiob_out);
        }
        
        vTaskDelay(pdMS_TO_TICKS(2500));

        // -------------------------------------------------------------------------------------------------------------------

        // Leer valor del canal 0 del ADS1115
        int16_t adc_value = 0;
        err = i2c_ads1115_read_single_ended(ADS1115_CHANNEL_0, &adc_value);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read ADS1115 channel 0: %s", esp_err_to_name(err));
        } 
        else 
        {
            float voltage = (adc_value * 2.048f) / 32768.0f; // Convertir a voltaje (PGA ±2.048V)
            ESP_LOGI(TAG, "ADS1115 Channel 0: Raw=%d, Voltage=%.4f V", adc_value, voltage);
        }

        vTaskDelay(pdMS_TO_TICKS(2500));

        // -------------------------------------------------------------------------------------------------------------------
        
        rtos_print_task_list();
    }
}
