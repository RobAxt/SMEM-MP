#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "security_module.h"
#include "ambiental_module.h"
#include "communication_module.h"

#define LOOP_DELAY_MS 60000

static const char *TAG = "app_main";

void app_main(void) 
{
    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_LOGI(TAG, "Initializing SMEM-MP application");
    
    // Application code begins here

    esp_ip4_addr_t ip     = { .addr = ESP_IP4TOADDR(192, 168, 160, 2) };
    esp_ip4_addr_t gw     = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };
    esp_ip4_addr_t mask   = { .addr = ESP_IP4TOADDR(255, 255, 255, 0) };
    esp_ip4_addr_t ntp    = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };
    esp_ip4_addr_t broker = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };

    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(security_module_start());

    ESP_ERROR_CHECK(ambiental_module_start());

    ESP_ERROR_CHECK(communication_module_start(ip, gw, mask, ntp, broker));

    // Application code ends here
 
    while (1)
    {
        ESP_LOGD(TAG, "SMEM-MP is running...");

        // Application code begins here
        
        ESP_LOGI("HEAP", "Free heap: %u bytes", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
        ESP_LOGI("HEAP", "Largest free block: %u bytes", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
        ESP_LOGI("HEAP", "Minimum ever free: %u bytes", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
        
        // Application code ends here
        
        vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
    }
}