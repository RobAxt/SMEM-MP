#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "net_driver.h"
#include "sntp_driver.h"
#include "mqtt_driver.h"
#include "time_publisher.h"

static const char *TAG = "app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SMEM-MP application");
    
    esp_ip4_addr_t ip     = { .addr = ESP_IP4TOADDR(192, 168, 160, 2) };
    esp_ip4_addr_t gw     = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };
    esp_ip4_addr_t mask   = { .addr = ESP_IP4TOADDR(255, 255, 255, 0) };
    esp_ip4_addr_t ntp  = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };
    esp_ip4_addr_t broker = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };

    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(eth_net_start(ip, gw, mask));

    ESP_ERROR_CHECK(eth_net_ready());
    
    ESP_ERROR_CHECK(sntp_client_start(ntp));

    ESP_ERROR_CHECK(mqtt_client_start(broker));

    ESP_ERROR_CHECK(time_publisher_start());

    ESP_LOGI(TAG, "Ending app_main function");
}
