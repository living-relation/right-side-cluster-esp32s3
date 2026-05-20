/**
 * main.c — TrackCluster RIGHT cluster (ESP32-S3, 480×480)
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "TCA9554PWR.h"
#include "ST7701S.h"
#include "GT911.h"
#include "LVGL_Driver.h"
#include "boot/boot_screen.h"
#include "ui/screens/ui_Screen1.h"
#include "ui/ui.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "right-dash_data.h"
#include "right-colors.h"
#include "dash_sync.h"

#define UART_PORT       UART_NUM_1
#define UART_RX_PIN     18
#define UART_TX_PIN     17

static const char *TAG = "RIGHT";

extern volatile dash_data_t dash;

portMUX_TYPE g_dash_mux = portMUX_INITIALIZER_UNLOCKED;

static void uart_init(void) {
    uart_config_t cfg = {
        .baud_rate  = UART_BRIDGE_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                                  UART_TX_PIN,
                                  UART_RX_PIN,
                                  UART_PIN_NO_CHANGE,
                                  UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 4096, 0, 0, NULL, 0));

    ESP_LOGI(TAG, "UART initialized (921600 8N1, RX on GPIO %d)", UART_RX_PIN);
}

static void uart_rx_task(void *arg) {
    uint8_t buf[UART_BRIDGE_FRAME_LEN];
    int idx = 0;
    uint16_t seq;

    while (1) {
        uint8_t byte;
        if (uart_read_bytes(UART_PORT, &byte, 1, portMAX_DELAY) != 1)
            continue;

        if (idx == 0 && byte != UART_BRIDGE_SOF)
            continue;

        buf[idx++] = byte;

        if (idx == UART_BRIDGE_FRAME_LEN) {
            idx = 0;

            dash_data_t tmp;
            if (dash_decode_right(buf, &tmp, &seq)) {
                tmp.last_update_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);

                portENTER_CRITICAL(&g_dash_mux);
                memcpy((void *)&dash, &tmp, sizeof(dash_data_t));
                portEXIT_CRITICAL(&g_dash_mux);
            } else {
                ESP_LOGW(TAG, "CRC/frame fail");
            }
        }
    }
}

typedef enum {
    STALE_NORMAL,
    STALE_FADING,
    STALE_NO_ECU,
} stale_state_t;

static stale_state_t check_staleness(void) {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
    uint32_t age = now - dash.last_update_ms;

    if (dash.last_update_ms == 0) return STALE_NO_ECU;
    if (age <= DASH_STALE_MS)     return STALE_NORMAL;
    if (age <= DASH_STALE_OFF_MS) return STALE_FADING;
    return STALE_NO_ECU;
}

static void paint_timer_cb(lv_timer_t *t) {
    (void)t;

    stale_state_t ss = check_staleness();

    if (ss == STALE_NO_ECU) {
        lv_obj_clear_flag(ui_no_ecu_pill, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_no_ecu_label, "NO ECU");
        lv_obj_set_style_opa(ui_Screen1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }
    if (ss == STALE_FADING) {
        lv_obj_clear_flag(ui_no_ecu_pill, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_no_ecu_label, "WAITING FOR ECU");
    } else {
        lv_obj_add_flag(ui_no_ecu_pill, LV_OBJ_FLAG_HIDDEN);
    }

    dash_data_t snap;
    portENTER_CRITICAL(&g_dash_mux);
    memcpy(&snap, (const void *)&dash, sizeof(dash_data_t));
    portEXIT_CRITICAL(&g_dash_mux);

    lv_opa_t content_opa = (ss == STALE_FADING) ? LV_OPA_30 : LV_OPA_COVER;
    lv_obj_set_style_opa(ui_Screen1, content_opa, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (ss == STALE_FADING)
        lv_obj_set_style_opa(ui_no_ecu_pill, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Lambda */
    char lam_str[8];
    snprintf(lam_str, sizeof(lam_str), "%.3f", snap.lambda);
    lv_label_set_text(ui_label_lambda_val, lam_str);
    lv_color_t lam_c = lambda_color(snap.lambda);
    lv_obj_set_style_bg_color(ui_lambda_band, lam_c, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_label_lambda_val, lam_c, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Boost */
    int boost_v = (int)snap.boost;
    if (boost_v < 0) boost_v = 0;
    if (boost_v > 35) boost_v = 35;
    lv_bar_set_value(ui_bar_boost, boost_v, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ui_bar_boost, boost_color(snap.boost),
                              LV_PART_INDICATOR | LV_STATE_DEFAULT);
    float boost_disp = snap.boost < 0.0f ? 0.0f : (snap.boost > 35.0f ? 35.0f : snap.boost);
    char boost_str[8];
    snprintf(boost_str, sizeof(boost_str), "%.1f", boost_disp);
    lv_label_set_text(ui_label_boost_val, boost_str);

    /* ECT */
    int ect_v = (int)snap.coolant_temp;
    if (ect_v < 100) ect_v = 100;
    if (ect_v > 300) ect_v = 300;
    lv_bar_set_value(ui_bar_ect, ect_v, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ui_bar_ect, ect_color(snap.coolant_temp),
                              LV_PART_INDICATOR | LV_STATE_DEFAULT);
    char ect_str[8];
    snprintf(ect_str, sizeof(ect_str), "%d", (int)snap.coolant_temp);
    lv_label_set_text(ui_label_ect_val, ect_str);

    /* IGN */
    int ign_v = (int)snap.ign_adv;
    if (ign_v < 0) ign_v = 0;
    if (ign_v > 45) ign_v = 45;
    lv_bar_set_value(ui_bar_ign, ign_v, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ui_bar_ign, ign_color(snap.ign_adv),
                              LV_PART_INDICATOR | LV_STATE_DEFAULT);
    float ign_disp = snap.ign_adv < 0.0f ? 0.0f : (snap.ign_adv > 45.0f ? 45.0f : snap.ign_adv);
    char ign_str[8];
    snprintf(ign_str, sizeof(ign_str), "%.1f", ign_disp);
    lv_label_set_text(ui_label_ign_val, ign_str);

    /* IAT */
    int iat_v = (int)snap.iat;
    if (iat_v < 30)  iat_v = 30;
    if (iat_v > 200) iat_v = 200;
    lv_bar_set_value(ui_bar_iat, iat_v, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ui_bar_iat, iat_color(snap.iat),
                              LV_PART_INDICATOR | LV_STATE_DEFAULT);
    char iat_str[8];
    snprintf(iat_str, sizeof(iat_str), "%d", (int)snap.iat);
    lv_label_set_text(ui_label_iat_val, iat_str);
}

#define RIGHT_LOCAL_FLAGS (DASH_FLAG_LAMBDA_BAD | DASH_FLAG_OVERBOOST | DASH_FLAG_COOLANT_HOT)

static void alarm_task(void *arg) {
    (void)arg;

    while (1) {
        portENTER_CRITICAL(&g_dash_mux);
        dash_data_t snap;
        memcpy(&snap, (const void *)&dash, sizeof(dash_data_t));
        portEXIT_CRITICAL(&g_dash_mux);

        uint16_t flags = 0;

        if (snap.lambda < DASH_LAMBDA_RICH_ALARM ||
            snap.lambda > DASH_LAMBDA_LEAN_ALARM)
            flags |= DASH_FLAG_LAMBDA_BAD;

        if (snap.boost > DASH_BOOST_OVERBOOST)
            flags |= DASH_FLAG_OVERBOOST;

        if (snap.coolant_temp > DASH_COOLANT_MAX)
            flags |= DASH_FLAG_COOLANT_HOT;

        portENTER_CRITICAL(&g_dash_mux);
        dash.flags = (dash.flags & ~RIGHT_LOCAL_FLAGS) | flags;
        portEXIT_CRITICAL(&g_dash_mux);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void app_main(void) {
    I2C_Init();
    EXIO_Init();
    LCD_Init();
    LVGL_Init();

    ui_init();
    uart_init();

    xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL,
                configMAX_PRIORITIES - 2, NULL);

    xTaskCreate(alarm_task, "alarm", 2048, NULL,
                configMAX_PRIORITIES - 3, NULL);

    lv_timer_create(paint_timer_cb, 33, NULL);

    Set_Backlight(0);
    vTaskDelay(pdMS_TO_TICKS(750));
    Set_Backlight(100);
}
