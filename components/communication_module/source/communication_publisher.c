#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cJSON.h"

#include "communication_module.h"
#include "communication_suscriber.h"
#include "mqtt_driver.h"
#include "sntp_driver.h"

#define PERIOD_MS 30000
#define PAYLOAD_SIZE 64
#define TOPIC "TGN/Ferreyra/Comunicaciones/EMyR/N2440/TIME"

static const char *TAG = "communication_publisher";
static TaskHandle_t s_time_pub_handle = NULL;

static void time_publisher_task(void *arg)
{
    char json_buf[PAYLOAD_SIZE];
    char time_str[ISO_TIMESTAMP_SIZE];

    while (1)
    {
        if (sntp_client_isotime(time_str, sizeof(time_str)) == ESP_OK)
        {
            snprintf(json_buf, sizeof(json_buf), "{\"timestamp\":\"%s\"}", time_str);
            ESP_LOGI(TAG, "Publish: %s", json_buf);
            mqtt_client_publish(TOPIC, json_buf, 0);
            
        }
        else
        {
            ESP_LOGW(TAG, "Failed to get time");
        }

        vTaskDelay(pdMS_TO_TICKS(PERIOD_MS));
    }
}

esp_err_t communication_publisher_start(void)
{
    if (s_time_pub_handle != NULL)
    {
        ESP_LOGW(TAG, "Task already running");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(time_publisher_task, "time_publisher_task", 4096, NULL, tskIDLE_PRIORITY + 5, &s_time_pub_handle);
    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create time publisher task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Time publisher started");
    return ESP_OK;
}
