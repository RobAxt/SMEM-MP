#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "DummyComponent.h"
#include "net_driver.h"

static const char *TAG = "SMEM-MP";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SMEM-MP application");
    
    esp_net_start();

    esp_net_ready();

    while(1) {
        ESP_LOGI(TAG, "Running main loop");
        // Simulate some work by calling the dummy component function
        func();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
