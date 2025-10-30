#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "security_module.h"

static const char *TAG = "SMEM-MP";

void app_main(void) 
{
    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_LOGI(TAG, "Starting SMEM-MP application...");

    // Application code begins here

    ESP_ERROR_CHECK(security_module_start());
    
    // Application code ends here
    
    while (1)
    {
        ESP_LOGD(TAG, "SMEM-MP is running...");
        // Application code begins here
        ESP_LOGI("HEAP", "Free heap: %u bytes", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
        ESP_LOGI("HEAP", "Largest free block: %u bytes", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
        ESP_LOGI("HEAP", "Minimum ever free: %u bytes", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));

        // Application code ends here
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}