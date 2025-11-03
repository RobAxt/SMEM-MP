#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "uart_pzem004t.h"

#define LOOP_DELAY_MS 5000

static const char *TAG = "SMEM-MP";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SMEM-MP application");

    // Application code begins here

    ESP_ERROR_CHECK(uart_pzem004t_start(UART_NUM_1, GPIO_NUM_18, GPIO_NUM_19));

    // Application code ends here
 
    while (1)
    {
        ESP_LOGD(TAG, "SMEM-MP is running...");

        // Application code begins here
        
        esp_err_t err = uart_pzem004t_read();
        if(err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Failed to read from PZEM004T: %s", esp_err_to_name(err));
        }

    //    ESP_LOGI("HEAP", "Free heap: %u bytes", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    //    ESP_LOGI("HEAP", "Largest free block: %u bytes", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
    //    ESP_LOGI("HEAP", "Minimum ever free: %u bytes", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
        
        // Application code ends here
        
        vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
    }
}
