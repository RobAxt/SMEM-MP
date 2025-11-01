#include "esp_log.h"

#include "net_driver.h"
#include "sntp_driver.h"
#include "mqtt_driver.h"
#include "communication_module.h"
#include "communication_suscriber.h"
#include "communication_publisher.h"

const char MQTT_BASE_TOPIC[MQTT_BASE_TOPIC_SIZE] = "TGN/Ferreyra/Comunicaciones/EMyR/N2440/";

static const char *TAG = "communication_module";

esp_err_t communication_module_start(esp_ip4_addr_t ip, esp_ip4_addr_t gw, esp_ip4_addr_t mask, esp_ip4_addr_t ntp, esp_ip4_addr_t broker)
{
    ESP_LOGI(TAG, "Starting communication module");

    esp_err_t ret;

    ret = eth_net_start(ip, gw, mask);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start Ethernet network: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = eth_net_ready();
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Ethernet network not ready: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = sntp_client_start(ntp);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start SNTP client: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = mqtt_client_start(broker);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = communication_suscriber_start();
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start communication subscriber: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = communication_publisher_start();
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start communication publisher: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Communication module started successfully");
    return ESP_OK;
}
