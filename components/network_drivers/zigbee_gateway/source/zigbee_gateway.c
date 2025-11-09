// components/network_drivers/zigbee_gateway/source/zigbee_gateway.c

#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "esp_zigbee_core.h"               // core types, init/start, signal handler
#include "platform/esp_zigbee_platform.h"           // ESP_ZB_DEFAULT_*_CONFIG
#include "zcl/esp_zigbee_zcl_common.h"     // ZCL common
#include "zcl/esp_zigbee_zcl_command.h"    // ZCL commands (read/write/report)

// ---------- Public config types (declared in your headers) ----------
#include "zigbee_gateway.h" 
// ---------- Logging ----------
static const char *TAG = "zigbee_gateway";

// ---------- Internal state ----------
typedef struct {
    zigbee_gateway_config_t cfg;   // Copy of user configuration
    esp_timer_handle_t poll_timer;
    bool started;
} zigbee_gateway_ctx_t;

static zigbee_gateway_ctx_t s_ctx = {0};

// ---------- Forward declarations ----------
static void app_signal_handler(esp_zb_app_signal_t *signal_struct);
static void zcl_cmd_cb(const esp_zb_zcl_cmd_message_t *cmd);
static void on_zcl_report_attr(const esp_zb_zcl_report_attr_message_t *msg);
static void on_zcl_read_attr_resp(const esp_zb_zcl_cmd_read_attr_resp_message_t *msg);
static void register_zcl_handlers_(void);
static void poll_timer_cb(void *arg);

// ============================================================
// ZCL helpers
// ============================================================

/**
 * @brief Handle ZCL "Report Attributes" messages.
 *
 * Notes for esp-zigbee-lib 1.6.0:
 * - source short address field is exposed via `src_addr_u.short_addr`
 * - attribute has fields `.type` and `.data` (not `.data_type` / `.data_p`)
 */
static void on_zcl_report_attr(const esp_zb_zcl_report_attr_message_t *msg)
{
    if (!msg) {
        ESP_LOGW(TAG, "Report attr: null msg");
        return;
    }

    uint16_t src_short = msg->src_addr_u.short_addr; // union with short_addr
    uint8_t  src_ep    = msg->src_endpoint;
    uint16_t cluster   = msg->cluster;
    uint16_t attr_id   = msg->attribute.id;
    uint8_t  dtype     = msg->attribute.type;

    // Basic typed decoding example (boolean and uint8)
    if (dtype == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
        bool state = (*(const uint8_t *)msg->attribute.data) ? true : false;
        ESP_LOGI(TAG, "ZCL Report: src=0x%04x ep=%u cluster=0x%04x attr=0x%04x (bool)=%d",
                 src_short, src_ep, cluster, attr_id, state);
        // TODO: dispatch to your application (publish to MQTT, AO event, etc.)
        return;
    }

    if (dtype == ESP_ZB_ZCL_ATTR_TYPE_U8) {
        uint8_t v = *(const uint8_t *)msg->attribute.data;
        ESP_LOGI(TAG, "ZCL Report: src=0x%04x ep=%u cluster=0x%04x attr=0x%04x (u8)=%u",
                 src_short, src_ep, cluster, attr_id, v);
        // TODO: dispatch
        return;
    }

    // Fallback: log hex payload
    ESP_LOGI(TAG, "ZCL Report: src=0x%04x ep=%u cluster=0x%04x attr=0x%04x type=0x%02x",
             src_short, src_ep, cluster, attr_id, dtype);
    // TODO: add other decoders depending on your channels map
}

/**
 * @brief Handle ZCL Read Attributes Response.
 *
 * For esp-zigbee-lib 1.6.0 the response message is:
 *   esp_zb_zcl_cmd_read_attr_resp_message_t
 * and contains:
 *   - attr_count
 *   - attr_list[] of esp_zb_zcl_read_attr_resp_record_t
 * Each record has `status` and `attr` with `.id`, `.type`, `.data`.
 */
static void on_zcl_read_attr_resp(const esp_zb_zcl_cmd_read_attr_resp_message_t *msg)
{
    if (!msg) return;

    for (int i = 0; i < msg->attr_count; ++i) {
        const esp_zb_zcl_read_attr_resp_record_t *r = &msg->attr_list[i];
        if (r->status != ESP_ZB_ZCL_STATUS_SUCCESS) {
            ESP_LOGW(TAG, "ReadAttrResp[%d]: status=0x%02x", i, r->status);
            continue;
        }
        uint16_t id    = r->attr.id;
        uint8_t  type  = r->attr.type;

        if (type == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
            bool b = (*(const uint8_t *)r->attr.data) ? true : false;
            ESP_LOGI(TAG, "ReadAttrResp: attr=0x%04x (bool)=%d", id, b);
        } else if (type == ESP_ZB_ZCL_ATTR_TYPE_U8) {
            uint8_t v = *(const uint8_t *)r->attr.data;
            ESP_LOGI(TAG, "ReadAttrResp: attr=0x%04x (u8)=%u", id, v);
        } else {
            ESP_LOGI(TAG, "ReadAttrResp: attr=0x%04x type=0x%02x", id, type);
        }
        // TODO: feed your AO/FSM or publish result
    }
}

/**
 * @brief Unified ZCL callback entry point registered with Zigbee stack.
 *        Dispatches sub-types: report, read-attr-response, etc.
 */
static void zcl_cmd_cb(const esp_zb_zcl_cmd_message_t *cmd)
{
    if (!cmd) return;

    switch (cmd->common.cmd_id) {
        case ESP_ZB_ZCL_CMD_REPORT_ATTR_ID: {
            const esp_zb_zcl_report_attr_message_t *rep =
                (const esp_zb_zcl_report_attr_message_t *)cmd;
            on_zcl_report_attr(rep);
            break;
        }
        case ESP_ZB_ZCL_CMD_READ_ATTR_RESP_ID: {
            const esp_zb_zcl_cmd_read_attr_resp_message_t *resp =
                (const esp_zb_zcl_cmd_read_attr_resp_message_t *)cmd;
            on_zcl_read_attr_resp(resp);
            break;
        }
        default:
            ESP_LOGD(TAG, "Unhandled ZCL cmd_id=0x%02x cluster=0x%04x ep=%u",
                     cmd->common.cmd_id, cmd->common.cluster, cmd->common.endpoint);
            break;
    }
}

// ============================================================
// Commissioning / Signals
// ============================================================

/**
 * @brief Zigbee application signal handler.
 *        Use NETWORK_* enums for BDB commissioning in 1.6.0.
 */
static void app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    esp_zb_app_signal_type_t sig = *(esp_zb_app_signal_type_t *)signal_struct->p_app_signal;
    esp_err_t status = signal_struct->esp_err_status;

    switch (sig) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:     // stack ready to commission
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT: {
        ESP_LOGI(TAG, "Stack ready (sig=%d), status=%s", sig, esp_err_to_name(status));
        // Start network formation for coordinator or steering for router/end-device
        // Your role should be set by configuration (coordinator/router).
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_NETWORK_FORMATION);
        break;
    }
    case ESP_ZB_BDB_SIGNAL_FORMATION: {
        if (status == ESP_OK) {
            ESP_LOGI(TAG, "Network formed. Starting steering...");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_NETWORK_STEERING);
        } else {
            ESP_LOGW(TAG, "Formation failed, retrying...");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_NETWORK_FORMATION);
        }
        break;
    }
    case ESP_ZB_BDB_SIGNAL_STEERING: {
        ESP_LOGI(TAG, "Steering done, status=%s", esp_err_to_name(status));
        break;
    }
    default:
        ESP_LOGD(TAG, "Unhandled signal=%d status=%s", sig, esp_err_to_name(status));
        break;
    }
}

// ============================================================
// Periodic poll (example): read a single attribute from map
// ============================================================

static void poll_timer_cb(void *arg)
{
    // Example: read the first configured channel periodically
    if (s_ctx.cfg.ch_count == 0) return;

    const int ch = 0;
    uint16_t short_addr = s_ctx.cfg.ch_map[ch].short_addr;

    // Build ZCL Read Attribute Command for 1.6.0 (note nested zcl_basic_cmd)
    esp_zb_zcl_read_attr_cmd_t cmd = {
        .zcl_basic_cmd = {
            .dst_addr_u.short_addr = short_addr,
            .dst_endpoint          = s_ctx.cfg.ch_map[ch].endpoint,
            .src_endpoint          = s_ctx.cfg.src_endpoint, // provided by your cfg
            .address_mode          = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        },
        .clusterID  = s_ctx.cfg.ch_map[ch].cluster_id,
        .attr_field = { .id = s_ctx.cfg.ch_map[ch].attribute_id },
    };

    uint8_t tsn = esp_zb_zcl_read_attr_cmd_req(&cmd);
    ESP_LOGD(TAG, "poll read sent tsn=%u short=0x%04x cluster=0x%04x attr=0x%04x",
             tsn, short_addr, s_ctx.cfg.ch_map[ch].cluster_id, s_ctx.cfg.ch_map[ch].attribute_id);
}

// ============================================================
// Public API
// ============================================================

esp_err_t zigbee_gateway_init(const zigbee_gateway_cfg_t *cfg)
{
    // Initialize NVS once (required by Zigbee radio/stack)
    ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "nvs_flash_init failed");

    // Copy user configuration
    memset(&s_ctx, 0, sizeof(s_ctx));
    if (cfg) {
        s_ctx.cfg = *cfg; // shallow copy; ensure all pointers in cfg are valid
    }

    // Platform config (radio + host)
    esp_zb_platform_config_t platform_cfg = {
        .radio_config = (esp_zb_radio_config_t) ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config  = (esp_zb_host_config_t)  ESP_ZB_DEFAULT_HOST_CONFIG(),
    };

    ESP_RETURN_ON_ERROR(esp_zb_platform_config(&platform_cfg), TAG, "platform_config failed");

    // Zigbee device configuration: channel mask and role from your cfg
    esp_zb_cfg_t zb_cfg = {
        .esp_zb_role = s_ctx.cfg.role, // e.g. ESP_ZB_DEVICE_TYPE_COORDINATOR
        .nwk_cfg = {
            .zigbee_channel_mask = s_ctx.cfg.channel_mask,
        },
    };

    ESP_RETURN_ON_ERROR(esp_zb_init(&zb_cfg), TAG, "esp_zb_init failed");

    // Register signal and ZCL callbacks
    esp_zb_app_register_signal_handler(app_signal_handler);
    register_zcl_handlers_();

    // Optional: create a periodic poll timer if requested
    if (s_ctx.cfg.poll_period_ms > 0) {
        const esp_timer_create_args_t args = {
            .callback = &poll_timer_cb,
            .name     = "zb_poll"
        };
        ESP_RETURN_ON_ERROR(esp_timer_create(&args, &s_ctx.poll_timer), TAG, "timer create");
        ESP_RETURN_ON_ERROR(esp_timer_start_periodic(s_ctx.poll_timer, (uint64_t)s_ctx.cfg.poll_period_ms * 1000ULL),
                            TAG, "timer start");
    }

    // Start Zigbee stack (non-blocking)
    ESP_RETURN_ON_ERROR(esp_zb_start(false), TAG, "esp_zb_start failed");
    s_ctx.started = true;
    ESP_LOGI(TAG, "Zigbee gateway initialized.");
    return ESP_OK;
}

esp_err_t zigbee_gateway_permit_join(uint8_t seconds)
{
    // In 1.6.0 use ZDO "permit joining request"
    esp_zb_zdo_permit_joining_req_param_t p = {
        .dst_nwk_addr    = 0xFFFC,      // broadcast (mgmt permit join)
        .permit_duration = seconds,
        .tc_significance = 0,
    };
    return esp_zb_zdo_permit_joining_req(&p);
}

esp_err_t zigbee_gateway_read_once(uint16_t short_addr, uint8_t endpoint,
                                   uint16_t cluster_id, uint16_t attribute_id)
{
    esp_zb_zcl_read_attr_cmd_t cmd = {
        .zcl_basic_cmd = {
            .dst_addr_u.short_addr = short_addr,
            .dst_endpoint          = endpoint,
            .src_endpoint          = s_ctx.cfg.src_endpoint,
            .address_mode          = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        },
        .clusterID  = cluster_id,
        .attr_field = { .id = attribute_id },
    };

    (void)esp_zb_zcl_read_attr_cmd_req(&cmd);
    return ESP_OK;
}

// ============================================================
// Registration helpers
// ============================================================

static void register_zcl_handlers_(void)
{
    // Register a single entry point that internally dispatches on cmd_id
    esp_zb_app_register_zcl_callback(zcl_cmd_cb);
}
