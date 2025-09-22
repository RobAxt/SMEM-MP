#include "i2c_ads1115.h"
#include "i2c_mgmt_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* ==== Registros ADS1115 ==== */
#define ADS1115_REG_CONVERSION   0x00
#define ADS1115_REG_CONFIG       0x01

/* ==== Bits de CONFIG (datasheet) ==== */
#define ADS1115_OS_SINGLE        (1u << 15)      // Start single conversion / OS bit
/* MUX single-ended (AINx-GND) en [14:12]: 100,101,110,111 */
static inline uint16_t mux_for_channel(int ch) {
    switch (ch) {
        case 0: return (0x4u << 12); // 100: AIN0 - GND
        case 1: return (0x5u << 12); // 101: AIN1 - GND
        case 2: return (0x6u << 12); // 110: AIN2 - GND
        case 3: return (0x7u << 12); // 111: AIN3 - GND
        default: return (0x4u << 12);
    }
}
/* PGA en [11:9] (según enum del header) */
static inline uint16_t cfg_bits_pga(ads1115_pga_t pga) {
    static const uint16_t map[] = {
        0x0000, // ±6.144 V
        0x0200, // ±4.096 V
        0x0400, // ±2.048 V
        0x0600, // ±1.024 V
        0x0800, // ±0.512 V
        0x0A00  // ±0.256 V
    };
    if (pga < ADS1115_PGA_6V144 || pga > ADS1115_PGA_0V256) pga = ADS1115_PGA_2V048;
    return map[pga];
}
/* Modo single-shot (bit 8 = 1) */
#define ADS1115_MODE_SINGLE      0x0100
/* DR en [7:5] (según enum del header) */
static inline uint16_t cfg_bits_dr(ads1115_dr_t dr) {
    if (dr < ADS1115_DR_8SPS) dr = ADS1115_DR_128SPS;
    if (dr > ADS1115_DR_860SPS) dr = ADS1115_DR_860SPS;
    return ((uint16_t)dr & 0x7u) << 5;
}
/* Comparator deshabilitado: cola=11b (bits [1:0]) y valores por defecto seguros */
#define ADS1115_COMP_DISABLE     0x0003

/* ==== Estado interno simple (un único ADS1115) ==== */
static const char *TAG = "i2c_ads1115";
static struct {
    uint8_t  addr;          // dirección I2C 7-bit (0x48..0x4B)
    uint16_t cfg_base;      // config base: PGA, DR, modo, comparator off (sin OS)
    int      timeout_ms;    // timeout por defecto para operaciones I2C/poll
    bool     ready;         // inicializado
} s_dev = {0};

/* ==== Helpers I2C ==== */
static esp_err_t write_u16(uint8_t addr, uint8_t reg, uint16_t val, int timeout_ms)
{
    uint8_t frame[3];
    frame[0] = reg;
    frame[1] = (uint8_t)((val >> 8) & 0xFF);
    frame[2] = (uint8_t)(val & 0xFF);
    return i2c_mgmt_write(addr, frame, sizeof(frame), timeout_ms);
}

static esp_err_t read_u16(uint8_t addr, uint8_t reg, uint16_t *out, int timeout_ms)
{
    esp_err_t err = i2c_mgmt_write(addr, &reg, 1, timeout_ms);
    if (err != ESP_OK) return err;

    uint8_t buf[2] = {0};
    size_t len = 2;
    err = i2c_mgmt_read(addr, buf, &len, timeout_ms);
    if (err != ESP_OK) return err;
    if (len != 2) return ESP_FAIL;

    *out = (uint16_t)((buf[0] << 8) | buf[1]);
    return ESP_OK;
}

/* Espera a que OS=1 (conversión completa); cede CPU en intervalos cortos */
static esp_err_t wait_conversion_ready(uint8_t addr, int timeout_ms)
{
    const int poll_step_ms = 1;
    int elapsed = 0;
    while (1) {
        uint16_t cfg = 0;
        esp_err_t err = read_u16(addr, ADS1115_REG_CONFIG, &cfg, timeout_ms);
        if (err != ESP_OK) return err;
        if (cfg & ADS1115_OS_SINGLE) return ESP_OK; // lista

        if (timeout_ms >= 0 && elapsed >= timeout_ms) return ESP_ERR_TIMEOUT;
        vTaskDelay(pdMS_TO_TICKS(poll_step_ms));
        elapsed += poll_step_ms;
    }
}

/* Dispara single-shot en canal dado con la cfg_base actual */
static esp_err_t trigger_single_shot(int channel, int timeout_ms)
{
    uint16_t cfg = s_dev.cfg_base | mux_for_channel(channel) | ADS1115_OS_SINGLE;
    return write_u16(s_dev.addr, ADS1115_REG_CONFIG, cfg, timeout_ms);
}

/* ==== API pública (según header dado) ==== */
esp_err_t i2c_ads1115_start(uint8_t i2c_addr, ads1115_pga_t pga, ads1115_dr_t dr, int timeout_ms)
{
    s_dev.addr = i2c_addr;
    s_dev.timeout_ms = timeout_ms;
    s_dev.cfg_base = ADS1115_MODE_SINGLE
                   | cfg_bits_pga(pga)
                   | cfg_bits_dr(dr)
                   | ADS1115_COMP_DISABLE;

    /* Pequeña verificación de comunicación: leer CONFIG */
    esp_err_t err = i2c_mgmt_begin_transaction();
    if (err != ESP_OK) return err;

    uint16_t cfg = 0;
    err = read_u16(s_dev.addr, ADS1115_REG_CONFIG, &cfg, s_dev.timeout_ms);

    {
        esp_err_t end_err = i2c_mgmt_end_transaction();
        if (err == ESP_OK) err = end_err;
    }

    if (err == ESP_OK) {
        s_dev.ready = true;
        ESP_LOGI(TAG, "ADS1115 @0x%02X: start OK (PGA=%d, DR=%d, timeout=%d ms)",
                 s_dev.addr, (int)pga, (int)dr, s_dev.timeout_ms);
    } else {
        s_dev.ready = false;
        ESP_LOGE(TAG, "ADS1115 @0x%02X: no responde (err=%s)",
                 s_dev.addr, esp_err_to_name(err));
    }
    return err;
}

esp_err_t i2c_ads1115_read_single_ended(ads1115_channel_t channel, int16_t *value)
{
    if (!s_dev.ready || !value) return ESP_ERR_INVALID_STATE;
    if (channel < ADS1115_CHANNEL_0 || channel > ADS1115_CHANNEL_3) return ESP_ERR_INVALID_ARG;

    esp_err_t err = i2c_mgmt_begin_transaction();
    if (err != ESP_OK) return err;

    /* 1) Dispara conversión en el canal */
    if ((err = trigger_single_shot((int)channel, s_dev.timeout_ms)) != ESP_OK) goto out;

    /* 2) Espera fin de conversión (OS=1) */
    if ((err = wait_conversion_ready(s_dev.addr, s_dev.timeout_ms)) != ESP_OK) goto out;

    /* 3) Lee el registro de conversión */
    {
        uint16_t u16 = 0;
        err = read_u16(s_dev.addr, ADS1115_REG_CONVERSION, &u16, s_dev.timeout_ms);
        if (err == ESP_OK) {
            *value = (int16_t)u16; // formato nativo del ADS1115
        }
    }

out:
    {
        esp_err_t end_err = i2c_mgmt_end_transaction();
        if (err == ESP_OK) err = end_err;
    }
    return err;
}
