#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "ambiental_module.h"

static const char *TAG = "SMEM-MP";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SMEM-MP application");

    ESP_ERROR_CHECK(ambiental_module_start());
 
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
