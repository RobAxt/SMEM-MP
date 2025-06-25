#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"

#include "net_driver.h"
#include "eth_driver.h"

static const char *TAG = "net_driver";
static SemaphoreHandle_t net_ready_smph = NULL;

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

esp_err_t esp_net_start(esp_ip4_addr_t ip, esp_ip4_addr_t gw, esp_ip4_addr_t mask)
{
    esp_err_t ret = ESP_OK;
    esp_eth_handle_t eth_handles = NULL;
    net_ready_smph = xSemaphoreCreateBinary();
    if (net_ready_smph == NULL) { 
        ESP_LOGE(TAG, "Error al crear el semáforo binario");
        return ESP_FAIL;
    }

    // Initialize Ethernet driver
    ESP_ERROR_CHECK(esp_eth_init(&eth_handles));
    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());   
    
    // Create instance of esp-netif for Ethernet
    // Use ESP_NETIF_DEFAULT_ETH when you don't need to modify default esp-netif configuration parameters.
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netifs = esp_netif_new(&cfg);

    // Stop DHCP client if it was started by default
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netifs));
    
    // Set static IP address for Ethernet interface
    esp_netif_ip_info_t ip_info = {
        .ip      = ip ,   // Set  static IP address
        .netmask = mask , // Set netmask
        .gw      = gw     // Set default gateway
    };
    ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netifs, &ip_info));

    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netifs, esp_eth_new_netif_glue(eth_handles)));

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    // Start Ethernet driver state machine
    ESP_ERROR_CHECK(esp_eth_start(eth_handles));

    return ret;
}

esp_err_t esp_net_ready(void)
{
    xSemaphoreTake(net_ready_smph, portMAX_DELAY); // Wait until the semaphore is given
    if (net_ready_smph != NULL) {
        vSemaphoreDelete(net_ready_smph); // Delete the semaphore after use
        net_ready_smph = NULL; // Set to NULL to avoid dangling pointer
    }
    ESP_LOGI(TAG, "Network is ready");
    return ESP_OK;
}

/** @brief Event handler for IP_EVENT_ETH_GOT_IP **/
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    eth_speed_t speed = ETH_SPEED_MAX;
    eth_duplex_t duplex = -1;
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        esp_eth_ioctl(eth_handle, ETH_CMD_G_SPEED, &speed);
        esp_eth_ioctl(eth_handle, ETH_CMD_G_DUPLEX_MODE, &duplex);

        ESP_LOGI(TAG, "Ethernet Link Up %s - %s",
                                speed == ETH_SPEED_100M ? "100 Mbps" : "10 Mbps",
                                duplex == ETH_DUPLEX_FULL ? "Full Duplex" : "Half Duplex" );
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
 // ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
 // const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    xSemaphoreGive(net_ready_smph);
}