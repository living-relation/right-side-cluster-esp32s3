/**
 * uart_rx.c — UART receive task, right cluster.
 *
 * Receives 32-byte RIGHT frames (FRAME_TYPE 0x02) from the center cluster.
 * Identical structure to left/uart_rx.c — only the decode call and the
 * fields written to g_dash differ.
 *
 * No sensor decoding. All data pre-processed by center.
 */

#include "dash_data.h"
#include "sdkconfig.h"
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "uart_rx";

#define RX_UART_PORT  UART_NUM_1
#define RX_BUF_SIZE   256

portMUX_TYPE g_dash_mux = portMUX_INITIALIZER_UNLOCKED;

static void uart_rx_init(void)
{
    uart_config_t cfg = {
        .baud_rate  = UART_BRIDGE_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(RX_UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(RX_UART_PORT,
                                 CONFIG_TC_UART_TX_GPIO,
                                 CONFIG_TC_UART_RX_GPIO,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(RX_UART_PORT, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_LOGI(TAG, "UART1 ready — baud=%u RX=GPIO%d",
             UART_BRIDGE_BAUD, CONFIG_TC_UART_RX_GPIO);
}

void uart_rx_task(void *arg)
{
    uart_rx_init();

    uint8_t  raw[UART_BRIDGE_FRAME_LEN * 2];
    int      raw_len  = 0;
    uint32_t err_count = 0;

    for (;;) {
        int n = uart_read_bytes(RX_UART_PORT,
                                raw + raw_len,
                                sizeof(raw) - raw_len,
                                pdMS_TO_TICKS(30));
        if (n > 0) raw_len += n;

        while (raw_len > 0 && raw[0] != UART_BRIDGE_SOF) {
            memmove(raw, raw + 1, --raw_len);
            err_count++;
        }

        if (raw_len < (int)UART_BRIDGE_FRAME_LEN) continue;

        uint8_t buf[UART_BRIDGE_FRAME_LEN];
        memcpy(buf, raw, UART_BRIDGE_FRAME_LEN);
        memmove(raw, raw + UART_BRIDGE_FRAME_LEN,
                raw_len - UART_BRIDGE_FRAME_LEN);
        raw_len -= UART_BRIDGE_FRAME_LEN;

        dash_data_t snap;
        uint16_t seq;
        if (!dash_decode_right(buf, &snap, &seq)) {
            if (++err_count % 100 == 0)
                ESP_LOGW(TAG, "%" PRIu32 " frame errors", err_count);
            continue;
        }

        portENTER_CRITICAL(&g_dash_mux);
        g_dash.lambda       = snap.lambda;
        g_dash.boost        = snap.boost;
        g_dash.coolant_temp = snap.coolant_temp;
        g_dash.ign_adv      = snap.ign_adv;
        g_dash.iat          = snap.iat;
        g_dash.rpm          = snap.rpm;
        g_dash.flags        = snap.flags;
        g_dash.menu_id      = snap.menu_id;
        g_dash.menu_cursor  = snap.menu_cursor;
        g_dash.last_update_ms =
            (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        portEXIT_CRITICAL(&g_dash_mux);
    }
}
