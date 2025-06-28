#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif_ip_addr.h"

#include "net_driver.h"

static const char *TAG = "app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SMEM-MP application");
    
    esp_ip4_addr_t ip = { .addr = ESP_IP4TOADDR(192, 168, 160, 2) };
    esp_ip4_addr_t gw = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };
    esp_ip4_addr_t mask = { .addr = ESP_IP4TOADDR(255, 255, 255, 0) };
    
    esp_net_start(ip, gw, mask);

    esp_net_ready();
    esp_net_ready();

    ESP_LOGI(TAG, "Ending app_main function");
}
