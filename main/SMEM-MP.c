#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif_ip_addr.h"

#include "DummyComponent.h"
#include "net_driver.h"

static const char *TAG = "SMEM-MP";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SMEM-MP application");
    
    esp_ip4_addr_t ip = { .addr = ESP_IP4TOADDR(192, 168, 160, 2) };
    esp_ip4_addr_t gw = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };
    esp_ip4_addr_t mask = { .addr = ESP_IP4TOADDR(255, 255, 255, 0) };
    
    esp_net_start(ip, gw, mask);

    esp_net_ready();

    while(1) {
        ESP_LOGI(TAG, "Running main loop");
        // Simulate some work by calling the dummy component function
        func();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
