#ifndef ZIGBEE_GATEWAY_H
#define ZIGBEE_GATEWAY_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"


/* ---- Public API for a 4-channel digital-input gateway ----
 *
 * One remote Zigbee node exposes 4 digital states as ZCL attributes.
 * This component forms a Zigbee network as Coordinator + Trust Center,
 * listens for incoming reports/read-responses, maps them to 4 logical channels,
 * and dispatches user-provided callbacks.
 */

/* Cluster & attribute IDs (ZCL) defaults */
#define ZB_GATEWAY_CLUSTER_ONOFF         0x0006
#define ZB_GATEWAY_ATTR_ONOFF            0x0000

typedef void (*zb_din_callback_t)(bool state,
                                  uint16_t src_short_addr,
                                  uint8_t src_endpoint,
                                  uint8_t channel_index);

/* Map for one logical channel:
 * Which endpoint/cluster/attribute on the remote device
 * correspond to this channel's digital state.
 */
typedef struct {
    uint8_t  endpoint;          /* remote endpoint */
    uint16_t cluster_id;        /* e.g., 0x0006 On/Off */
    uint16_t attribute_id;      /* e.g., 0x0000 OnOff */
} zb_channel_map_t;

/* Optional: periodic polling if reports are not configured on device */
typedef struct {
    bool     enable;            /* if true, the gateway will poll */
    uint32_t period_ms;         /* poll period (per channel) */
} zb_polling_cfg_t;

/* Zigbee Coordinator configuration */
typedef struct {
    /* Bitmask of Zigbee channels (11..26). Example: (1<<15)|(1<<20)|(1<<25) */
    uint32_t channel_mask;

    /* Optional fixed PAN ID; 0xFFFF means "auto" */
    uint16_t pan_id;

    /* Trust Center defaults (Zigbee 3.0); install-code policy can be added if needed */
    bool     allow_default_tc_link_key;

    /* 4 channels mapping (endpoint/cluster/attr) */
    zb_channel_map_t ch_map[4];

    /* Optional polling if reports are not used by the remote node */
    zb_polling_cfg_t polling;
} zigbee_gateway_config_t;

/* Initialize internal state only (no Zigbee start yet) */
esp_err_t zigbee_gateway_init(const zigbee_gateway_config_t *cfg);

/* Start Zigbee stack, form PAN, open steering once formed */
esp_err_t zigbee_gateway_start(void);

/* Enable/disable permit-join (open/close steering) for N seconds (0 = close) */
esp_err_t zigbee_gateway_permit_join(uint8_t seconds);

/* Assign per-channel callbacks. Passing NULL clears the callback. */
esp_err_t zigbee_gateway_set_callback(uint8_t ch_index, zb_din_callback_t cb);

/* Optional: read once (on demand) the attribute mapped to channel 'ch_index'.
 * This sends a Read Attributes command to the last-seen short address (if known)
 * or to a provided short address if 'dst_short_addr' != 0xFFFF.
 */
esp_err_t zigbee_gateway_read_once(uint8_t ch_index, uint16_t dst_short_addr);

/* Factory reset: clears Zigbee NVS and restarts formation next boot.
 * (Does not erase application NVS namespaces not used by Zigbee.)
 */
esp_err_t zigbee_gateway_factory_reset(void);

#endif // ZIGBEE_GATEWAY_H