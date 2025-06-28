#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif_ip_addr.h"

#include "net_driver.h"
#include "mqtt_driver.h"

static const char *TAG = "app_main";

static const char *TOPIC = "test/topic";

static void mqtt_handler(const char *topic, const char *payload) {
    ESP_LOGI(TAG, "Message received: %s", payload);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SMEM-MP application");
    
    esp_ip4_addr_t ip = { .addr = ESP_IP4TOADDR(192, 168, 160, 2) };
    esp_ip4_addr_t gw = { .addr = ESP_IP4TOADDR(192, 168, 160, 1) };
    esp_ip4_addr_t mask = { .addr = ESP_IP4TOADDR(255, 255, 255, 0) };
    
    ESP_ERROR_CHECK(nvs_flash_init());

    esp_net_start(ip, gw, mask);
    esp_net_ready();

    mqtt_client_start();
    mqtt_client_publish(TOPIC, "Hello, MQTT!", 0);
    mqtt_client_suscribe(TOPIC, mqtt_handler, 0);

    ESP_LOGI(TAG, "Ending app_main function");
}
