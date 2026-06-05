/**
 * main.c — right cluster application entry point.
 * No CAN. No sensor decoding. Data arrives over UART from center (normal mode).
 * Bench mode (CONFIG_TC_BENCH_MODE) runs bench_demo after boot splash completes.
 */

#include "bsp.h"
#include "dash_data.h"
#include "ui/ui.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "main";
extern void uart_rx_task(void *arg);
extern void alarm_task(void *arg);

void app_main(void)
{
    ESP_LOGI(TAG, "TrackCluster Right — booting");

    bsp_backlight_hold_off();
    ESP_ERROR_CHECK(bsp_init());
    lv_disp_t *disp = bsp_display_start();
    if (!disp) {
        ESP_LOGE(TAG, "display init failed — halting");
        for (;;) vTaskDelay(portMAX_DELAY);
    }

    bsp_lvgl_lock(portMAX_DELAY);
    ui_init(disp);
    bsp_lvgl_unlock();

#if CONFIG_TC_BENCH_MODE
    ESP_LOGW(TAG, "BENCH MODE — demo sequence starts after boot splash");
#else
    xTaskCreatePinnedToCore(uart_rx_task, "uart_rx", 4096, NULL, 7, NULL, 0);
#endif
    xTaskCreatePinnedToCore(alarm_task, "alarm", 4096, NULL, 6, NULL, 0);

    ESP_LOGI(TAG, "All tasks started");
}
