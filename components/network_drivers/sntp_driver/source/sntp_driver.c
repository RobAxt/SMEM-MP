#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/inet.h"
#include "sdkconfig.h"

#include "sntp_driver.h"

static const char *TAG = "sntp_driver";

void time_sync_notification_cb(struct timeval *tv);

esp_err_t sntp_client_start(esp_ip4_addr_t ip)
{
    ESP_LOGI(TAG, "Starting SNTP client...");
    esp_sntp_config_t config = {
        .smooth_sync = false,                     // Disable smooth synchronization
        .server_from_dhcp = false,                // Do not request NTP server configuration from DHCP
        .wait_for_sync = true,                    // Create a semaphore to signal time sync event
        .start = true,                            // Automatically start the SNTP service
        .sync_cb = time_sync_notification_cb,     // Set callback function on time sync event
        .renew_servers_after_new_IP = false,      // Do not refresh server list after new IP
        .ip_event_to_renew = IP_EVENT_ETH_GOT_IP, // Set the IP event ID to renew server list
        .index_of_first_server = 0,               // Refresh server list after the first server
        .num_of_servers = 1,                      // Number of preconfigured NTP servers
        .servers = { ip4addr_ntoa((const ip4_addr_t *)&ip) }  // List of NTP servers
    };

    setenv("TZ", "ART3", 1);
    tzset();

    esp_err_t ret = esp_netif_sntp_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SNTP client: %s", esp_err_to_name(ret));
        return ret;
    }

    int retry = 0;
    while (esp_netif_sntp_sync_wait(CONFIG_SNTP_RETRY_TIMEOUT_MS / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < CONFIG_SNTP_RETRY_COUNT) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, CONFIG_SNTP_RETRY_COUNT);
    }

    ESP_LOGD(TAG, "SNTP client initialized with server: %s", CONFIG_SNTP_TIME_SERVER);
    return ret;
}

esp_err_t sntp_client_time(char *datetime_string, size_t size)
{
    if (datetime_string == NULL || size < TIMESTAMP_STRING_SIZE) {
        ESP_LOGE(TAG, "Invalid datetime string buffer");
        return ESP_ERR_INVALID_ARG;
    }

    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(datetime_string, size, "%c", &timeinfo);

    ESP_LOGD(TAG, "Current time: %s", datetime_string);
    return ESP_OK;
}

esp_err_t sntp_client_isotime(char *datetime_string, size_t size)
{
    if (datetime_string == NULL || size < ISO_TIMESTAMP_SIZE) {
        return ESP_ERR_INVALID_ARG;
    }

    time_t now = time(NULL);
    if (now == ((time_t)-1)) {
        ESP_LOGE("sntp_client", "Failed to get current time");
        return ESP_FAIL;
    }

    struct tm timeinfo;
    if (localtime_r(&now, &timeinfo) == NULL) {
        ESP_LOGE("sntp_client", "Failed to convert time to UTC");
        return ESP_FAIL;
    }

    size_t len = strftime(datetime_string, size, "%Y-%m-%dT%H:%M:%S-03:00", &timeinfo);
    if (len == 0) {
        ESP_LOGE("sntp_client", "Failed to format time string");
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Callback function for time synchronization notifications.
 * @details This function is called when the SNTP client receives a time synchronization event.
 * It can be used to perform actions such as updating the system time or logging the event. 
 * 
 * @param tv Pointer to a timeval structure containing the new time.
 */
void time_sync_notification_cb(struct timeval *tv)
{
    time_t now = tv->tv_sec; 
    struct tm timeinfo;

    localtime_r(&now, &timeinfo);

    char time_str[TIMESTAMP_STRING_SIZE];
    strftime(time_str, sizeof(time_str), "%c", &timeinfo); 
    const char *server = esp_sntp_getservername(0); 

    ESP_LOGI(TAG, "Notification of a time synchronization event, querying SNTP server: %s", server ? server : "Unknown");
    ESP_LOGI(TAG, "New system time UTC: %s - Epoch timestamp: %ld", time_str,(long int) now);
}
