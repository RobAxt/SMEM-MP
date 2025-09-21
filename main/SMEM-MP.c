#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


static const char *TAG = "SMEM-MP";

void app_main(void) 
{
    esp_log_level_set("*", ESP_LOG_DEBUG);
    ESP_LOGI(TAG, "Starting SMEM-MP application...");
    // Application code here
    while (1)
    {
        ESP_LOGD(TAG, "SMEM-MP is running...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    ]
}
