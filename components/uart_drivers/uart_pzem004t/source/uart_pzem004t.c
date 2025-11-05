#include "esp_log.h"
#include "driver/uart.h"
#include "mbcontroller.h"

#include "uart_pzem004t.h"

#define MB_DEV_SPEED 9600
#define MB_RESPONSE_TIMEOUT_MS 2000
#define MB_RESPONSE_SIZE 20
#define MB_INIT_WAIT_MS 100
#define PZEM_SLAVE_ADDR 1

static const char *TAG = "uart_pzem004t";

static void *mbc_master_handle = NULL;

static float voltage_V = 0;
static float current_A = 0;
static float power_W   = 0;
static float energy_Wh = 0;
static float freq_Hz   = 0;
static float pf        = 0;

static inline uint32_t u16_from_regs(uint8_t lo, uint8_t hi)
{
    return ((uint32_t)hi << 8) | lo;
}

// Parse and print one PZEM frame (20 data bytes = 10 registers)
static void pzem_parse_and_log(const uint8_t *rx, size_t len)
{
    // Data Response frame should be:
    ESP_LOGD(TAG, "PZEM004T data received: %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x",
         rx[0], rx[1], rx[2], rx[3], rx[4], rx[5], rx[6], rx[7], rx[8], rx[9], rx[10], rx[11], rx[12], rx[13], rx[14], rx[15], rx[16], rx[17], rx[18], rx[19]);
    // Map per datasheet    
    voltage_V = (float)u16_from_regs(rx[0], rx[1]) * 0.1f;
    current_A = (float)u16_from_regs(rx[2], rx[3]) * 0.001f;
    power_W = (float)u16_from_regs(rx[6], rx[7]) * 0.1f;
    energy_Wh = (float)u16_from_regs(rx[8], rx[9]);
    freq_Hz = (float)u16_from_regs(rx[14], rx[15]) * 0.1f;
    pf = (float)u16_from_regs(rx[16], rx[17]) * 0.01f;

    ESP_LOGD(TAG, "V=%.1f V, I=%.3f A, P=%.1f W, E=%.0f Wh, f=%.1f Hz, PF=%.2f",
             voltage_V, current_A, power_W, energy_Wh, freq_Hz, pf);
}

esp_err_t uart_pzem004t_start(uart_port_t uart_num, gpio_num_t tx_io_num, gpio_num_t rx_io_num)
{
    esp_err_t err = ESP_OK;

    mb_communication_info_t comm_info = {
        .ser_opts.port = uart_num,
        .ser_opts.mode = MB_RTU,
        .ser_opts.baudrate = MB_DEV_SPEED,
        .ser_opts.parity = MB_PARITY_NONE,
        .ser_opts.uid = 0,
        .ser_opts.response_tout_ms = MB_RESPONSE_TIMEOUT_MS,
        .ser_opts.data_bits = UART_DATA_8_BITS,
        .ser_opts.stop_bits = UART_STOP_BITS_1
    };

    // Initialize Modbus controller
    err = mbc_master_create_serial(&comm_info, &mbc_master_handle);
    if (err != ESP_OK || mbc_master_handle == NULL) 
    {
        ESP_LOGE(TAG, "Failed to create Modbus master controller: %s", esp_err_to_name(err));
        return ESP_ERR_INVALID_STATE;
    }

    // Set UART pins
    err = uart_set_pin(uart_num, tx_io_num, rx_io_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(err));
        return err;
    }

    err = mbc_master_start(mbc_master_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start Modbus master: %s", esp_err_to_name(err));
        return err;
    }
    
    err = uart_set_mode(uart_num, UART_MODE_UART);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set UART mode: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "UART PZEM004T initialized successfully at UART1, TX:%d RX:%d", tx_io_num, rx_io_num);

    vTaskDelay(pdMS_TO_TICKS(MB_INIT_WAIT_MS));
    return ESP_OK;
}

esp_err_t uart_pzem004t_read(void)
{
    mb_param_request_t req = {
        .slave_addr = PZEM_SLAVE_ADDR,
        .command    = 0x04,      // Read Input Registers
        .reg_start  = 0x0000,
        .reg_size   = 10         // 10 registers: 0x0000..0x0009
    };

    // Reply buffer: addr(1)+func(1)+bytecnt(1)+data(20)+crc(2) = 25 bytes
    uint8_t rx[MB_RESPONSE_SIZE] = {0};
    esp_err_t err = mbc_master_send_request(mbc_master_handle, &req, rx);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "send_request failed: %s (0x%x)", esp_err_to_name(err), err);
        return err;
    }

    pzem_parse_and_log(rx, sizeof(rx));
    return ESP_OK;
}

esp_err_t uart_pzem_reset(void)
{
    mb_param_request_t req = {
        .slave_addr = PZEM_SLAVE_ADDR,
        .command    = 0x42,
        .reg_start  = 0,
        .reg_size   = 0
    };
    uint8_t rx[8] = {0};
    return mbc_master_send_request(mbc_master_handle, &req, rx);
}

inline float uart_pzem_voltage_V(void)
{
    return voltage_V;
}

inline float uart_pzem_current_A(void)
{
    return current_A;
}

inline float uart_pzem_power_W(void)
{
    return power_W;
}

inline float uart_pzem_energy_Wh(void)
{
    return energy_Wh;
}

inline float uart_pzem_freq_Hz(void)
{
    return freq_Hz;
}

inline float uart_pzem_pf(void)
{
    return pf;
}