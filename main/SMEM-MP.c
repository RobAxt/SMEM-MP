#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "DummyComponent.h"
#include "i2c_mgmt_driver.h"
#include "i2c_pn532.h"

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
    uint8_t uid[10] = {0};
    size_t uid_len = sizeof(uid);
    

    while(1)
    {
        ESP_LOGI(TAG, "Running main loop");
        // Simulate some work by calling the dummy component function
        uid_len = sizeof(uid);
        esp_err_t err = i2c_pn532_read_passive_target(uid, &uid_len);
        
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read passive target: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(5000)); // Wait before retrying
            continue;
        }
        
        if(uid_len > 0)
            ESP_LOGI(TAG, "Read passive target (%d) UID: %02X, %02X, %02X, %02X", (int)uid_len, uid[0], uid[1], uid[2], uid[3]);
        else
            ESP_LOGW(TAG, "No UID read, check if a tag is present");

        vTaskDelay(pdMS_TO_TICKS(5000));
        rtos_print_task_list();
    }
}
