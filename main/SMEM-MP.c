#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "DummyComponent.h"

static const char *TAG = "SMEM-MP";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SMEM-MP application");
 
    while(1) {
        ESP_LOGI(TAG, "Running main loop");
        // Simulate some work by calling the dummy component function
        func();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
