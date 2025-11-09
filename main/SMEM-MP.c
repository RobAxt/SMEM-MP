#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "zigbee_gateway.h"

static const char *TAG = "app_main";

/* ---- Your 4 callbacks ----
 * They can post to your FSM, publish MQTT, etc.
 */
static void cb_ch0(bool state, uint16_t src, uint8_t ep, uint8_t ch)
{ ESP_LOGI(TAG, "[CH%u] src=0x%04X ep=%u state=%s", ch, src, ep, state?"ON":"OFF"); }

static void cb_ch1(bool state, uint16_t src, uint8_t ep, uint8_t ch)
{ ESP_LOGI(TAG, "[CH%u] src=0x%04X ep=%u state=%s", ch, src, ep, state?"ON":"OFF"); }

static void cb_ch2(bool state, uint16_t src, uint8_t ep, uint8_t ch)
{ ESP_LOGI(TAG, "[CH%u] src=0x%04X ep=%u state=%s", ch, src, ep, state?"ON":"OFF"); }

static void cb_ch3(bool state, uint16_t src, uint8_t ep, uint8_t ch)
{ ESP_LOGI(TAG, "[CH%u] src=0x%04X ep=%u state=%s", ch, src, ep, state?"ON":"OFF"); }


void app_main(void)
{
    /* Map each logical channel to (endpoint, cluster, attribute)
     * Example: one remote endpoint = 10, 4 On/Off attributes on the same ep (common case).
     * If your device uses separate endpoints, change the .endpoint fields accordingly.
     */
    zigbee_gateway_config_t cfg = {
        .channel_mask = (1 << 15) | (1 << 20) | (1 << 25),
        .pan_id = 0xFFFF, /* auto */
        .allow_default_tc_link_key = true,
        .ch_map = {
            { .endpoint = 10, .cluster_id = ZB_GATEWAY_CLUSTER_ONOFF, .attribute_id = ZB_GATEWAY_ATTR_ONOFF }, // CH0
            { .endpoint = 10, .cluster_id = ZB_GATEWAY_CLUSTER_ONOFF, .attribute_id = ZB_GATEWAY_ATTR_ONOFF }, // CH1 (adjust if different)
            { .endpoint = 10, .cluster_id = ZB_GATEWAY_CLUSTER_ONOFF, .attribute_id = ZB_GATEWAY_ATTR_ONOFF }, // CH2
            { .endpoint = 10, .cluster_id = ZB_GATEWAY_CLUSTER_ONOFF, .attribute_id = ZB_GATEWAY_ATTR_ONOFF }, // CH3
        },
        .polling = {
            .enable = false,     /* set true if your remote does not report autonomously */
            .period_ms = 2000
        }
    };

    ESP_ERROR_CHECK(zigbee_gateway_init(&cfg));

    /* Assign callbacks from your app (can be changed at runtime) */
    zigbee_gateway_set_callback(0, cb_ch0);
    zigbee_gateway_set_callback(1, cb_ch1);
    zigbee_gateway_set_callback(2, cb_ch2);
    zigbee_gateway_set_callback(3, cb_ch3);

    ESP_ERROR_CHECK(zigbee_gateway_start());

    /* Optionally open join window manually (auto-steering happens after formation) */
     zigbee_gateway_permit_join(180);
}