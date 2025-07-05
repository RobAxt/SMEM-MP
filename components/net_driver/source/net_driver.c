#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"

#include "net_driver.h"
#include "eth_driver.h"

static const char *TAG = "net_driver";
static EventGroupHandle_t net_event_group = NULL;
#define NET_READY_BIT BIT0

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

esp_err_t eth_net_start(esp_ip4_addr_t ip, esp_ip4_addr_t gw, esp_ip4_addr_t mask)
{
    esp_err_t ret = ESP_OK;
    esp_eth_handle_t eth_handles = NULL;

    if (net_event_group == NULL) {
        net_event_group = xEventGroupCreate();
        if (net_event_group == NULL) {
            ESP_LOGE(TAG, "Error al crear el grupo de eventos");
            return ESP_FAIL;
        }
    } else {
        // Limpiar cualquier bit previo (por reinicio de interfaz)
        xEventGroupClearBits(net_event_group, NET_READY_BIT);
    }

    // Inicializar Ethernet y pila TCP/IP
    ESP_ERROR_CHECK(esp_eth_init(&eth_handles));
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netifs = esp_netif_new(&cfg);

    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netifs));

    esp_netif_ip_info_t ip_info = {
        .ip      = ip,
        .netmask = mask,
        .gw      = gw
    };
    ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netifs, &ip_info));
    ESP_ERROR_CHECK(esp_netif_attach(eth_netifs, esp_eth_new_netif_glue(eth_handles)));

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    ESP_ERROR_CHECK(esp_eth_start(eth_handles));

    return ret;
}

esp_err_t eth_net_ready(void)
{
    EventBits_t bits = xEventGroupWaitBits(net_event_group,
                                           NET_READY_BIT,
                                           pdFALSE,      // No limpiar el bit
                                           pdTRUE,       // Esperar todos los bits
                                           portMAX_DELAY);
    if (bits & NET_READY_BIT) {
        ESP_LOGI(TAG, "Network is ready");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Error esperando el bit de red");
    return ESP_FAIL;
}

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    eth_speed_t speed = ETH_SPEED_MAX;
    eth_duplex_t duplex = -1;
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        esp_eth_ioctl(eth_handle, ETH_CMD_G_SPEED, &speed);
        esp_eth_ioctl(eth_handle, ETH_CMD_G_DUPLEX_MODE, &duplex);

        ESP_LOGI(TAG, "Ethernet Link Up %s - %s",
                 speed == ETH_SPEED_100M ? "100 Mbps" : "10 Mbps",
                 duplex == ETH_DUPLEX_FULL ? "Full Duplex" : "Half Duplex");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2],
                 mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        // Podés limpiar el bit aquí si querés detectar reconexiones
        xEventGroupClearBits(net_event_group, NET_READY_BIT);
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

static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Ethernet Got IP Address");
    xEventGroupSetBits(net_event_group, NET_READY_BIT);
}
