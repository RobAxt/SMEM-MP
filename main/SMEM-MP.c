#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif_ip_addr.h"

#include "net_driver.h"
#include "sntp_driver.h"

static const char *TAG = "app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SMEM-MP application");
    
    esp_ip4_addr_t ip   = { .addr = ESP_IP4TOADDR(192, 168, 160, 2) };
    esp_ip4_addr_t gw   = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };
    esp_ip4_addr_t mask = { .addr = ESP_IP4TOADDR(255, 255, 255, 0) };
    esp_ip4_addr_t ntp  = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };

    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_net_start(ip, gw, mask));

    ESP_ERROR_CHECK(esp_net_ready());
    
    ESP_ERROR_CHECK(sntp_client_start(ntp));

    char datetime_string[ISO_TIMESTAMP_SIZE];
    esp_err_t err = sntp_client_isotime(datetime_string, sizeof(datetime_string));
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get current time: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Current time: %s", datetime_string);
    }

    ESP_LOGI(TAG, "Ending app_main function");
}
