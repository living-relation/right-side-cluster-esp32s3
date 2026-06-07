/**
 * ui.c — top-level LVGL UI coordinator, right cluster.
 *
 * Displays on 480×480 round:
 *   - Lambda digital block (λ glyph + Aerospace 56 px value, centered)
 *     Green 0.75–1.019 · Orange flanks · Red < 0.70 or ≥ 1.05
 *   - 4 horizontal bar gauges (lv_bar, LV_ANIM_OFF):
 *       Boost  0–35 PSI
 *       ECT    100–300 °F
 *       IGN    0–45 °BTDC
 *       IAT    30–200 °F
 *   (Standalone/bench: gauges always render g_dash — at 0 with nothing connected.)
 *
 * Visual reference: LVGL Simulator.html (right cluster) — kept in sync with this
 * code in both directions.
 */

#include "ui.h"
#include "bsp.h"
#include "ui_lambda.h"
#include "ui_bar_gauges.h"
#include "ui_menu_popup.h"
#include "ui_warning.h"
#include "ui_boot.h"
#include "bench_demo.h"
#include "ui_debug_log.h"
#include "ui_menu_popup.h"
#include "ui_signal_filter.h"
#include "dash_data.h"
#include "right-colors.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "esp_timer.h"

extern portMUX_TYPE g_dash_mux;
static lv_obj_t *s_screen = NULL;
static lv_obj_t *s_main_layer = NULL;
static bool s_live = false;

/* Overboost full-screen flash: latch ≥28 PSI, release <25 PSI (filtered boost). */
#define BOOST_BG_ON_PSI    28.0f
#define BOOST_BG_OFF_PSI   25.0f
#define BOOST_BG_FLASH_MS  100U

static bool     s_boost_bg_lat;
static bool     s_boost_bg_flash_on;
static uint32_t s_boost_bg_flash_ms;
static bool     s_dbg_first_paint;

#if CONFIG_TC_BENCH_MODE
#define UI_PAINT_TICK_MS   50U
#else
#define UI_PAINT_TICK_MS   5U
#endif

#define BOOT_LIVE_SETTLE_MS  400

static void ui_boost_bg_update(float boost_psi)
{
    if (!s_screen) {
        return;
    }

    if (!s_boost_bg_lat && boost_psi >= BOOST_BG_ON_PSI) {
        s_boost_bg_lat = true;
        s_boost_bg_flash_ms = 0;
    } else if (s_boost_bg_lat && boost_psi < BOOST_BG_OFF_PSI) {
        s_boost_bg_lat = false;
        s_boost_bg_flash_on = false;
        lv_obj_set_style_bg_color(s_screen, COLOR_BG_PRIMARY, 0);
        return;
    }

    if (!s_boost_bg_lat) {
        lv_obj_set_style_bg_color(s_screen, COLOR_BG_PRIMARY, 0);
        return;
    }

    s_boost_bg_flash_ms += UI_PAINT_TICK_MS;
    if (s_boost_bg_flash_ms >= BOOST_BG_FLASH_MS) {
        s_boost_bg_flash_ms = 0;
        s_boost_bg_flash_on = !s_boost_bg_flash_on;
        lv_obj_set_style_bg_color(s_screen,
                                  s_boost_bg_flash_on ? COLOR_YELLOW : COLOR_BG_PRIMARY,
                                  0);
    }
}

void ui_paint_tick(lv_timer_t *t)
{
    if (!s_live) return;

    // #region agent log
    if (!s_dbg_first_paint) {
        s_dbg_first_paint = true;
        UI_DBG_LOG("J3", "ui.c:ui_paint_tick", "first_paint",
                   "\"t_ms\":%lld", (long long)(esp_timer_get_time() / 1000LL));
    }
    // #endregion

    dash_data_t snap;
    portENTER_CRITICAL(&g_dash_mux);
    snap = *(const dash_data_t *)&g_dash;
    portEXIT_CRITICAL(&g_dash_mux);

    /* Always render the latest data (0 when nothing is connected — no overlay). */
    ui_lambda_update(&snap);
    ui_bar_gauges_update(&snap);
    ui_boost_bg_update(ui_filter_boost_last());
    ui_menu_popup_update(&snap);
    ui_warning_update(&snap);
}

static void ui_prime_snapshot(void)
{
    dash_data_t snap;
    portENTER_CRITICAL(&g_dash_mux);
    snap = *(const dash_data_t *)&g_dash;
    portEXIT_CRITICAL(&g_dash_mux);

    ui_lambda_update(&snap);
    ui_bar_gauges_update(&snap);
    ui_menu_popup_update(&snap);
    ui_warning_update(&snap);
}

static void ui_live_start_cb(lv_timer_t *t)
{
    lv_timer_delete(t);
    s_live = true;
    lv_timer_create(ui_paint_tick, UI_PAINT_TICK_MS, NULL);
}

static void ui_boot_handoff(void)
{
    if (!bsp_lvgl_lock(portMAX_DELAY)) {
        return;
    }
    if (s_main_layer) {
        lv_obj_remove_flag(s_main_layer, LV_OBJ_FLAG_HIDDEN);
    }
    ui_prime_snapshot();
    lv_refr_now(NULL);
    lv_refr_now(NULL);
    bsp_lvgl_unlock();
}

void ui_on_boot_complete(void)
{
#if CONFIG_TC_BENCH_MODE
    bench_demo_start();
#endif
    /* Let splash teardown finish before fast bar updates (reduces RGB tear). */
    lv_timer_t *t = lv_timer_create(ui_live_start_cb, BOOT_LIVE_SETTLE_MS, NULL);
    lv_timer_set_repeat_count(t, 1);
}

void ui_init(lv_disp_t *disp)
{
    s_screen = lv_disp_get_scr_act(disp);
    lv_obj_set_style_bg_color(s_screen, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);

    s_main_layer = lv_obj_create(s_screen);
    lv_obj_set_size(s_main_layer, BSP_LCD_H_RES, BSP_LCD_V_RES);
    lv_obj_center(s_main_layer);
    lv_obj_set_style_bg_opa(s_main_layer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_main_layer, 0, 0);
    lv_obj_set_style_radius(s_main_layer, 0, 0);
    lv_obj_clear_flag(s_main_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_main_layer, LV_OBJ_FLAG_HIDDEN);

    ui_lambda_create(s_main_layer);
    ui_bar_gauges_create(s_main_layer);
    ui_menu_popup_create(s_main_layer);
    ui_warning_create(s_main_layer);
    ui_boot_start(s_screen, ui_on_boot_complete, ui_boot_handoff);

    if (bsp_lvgl_lock(portMAX_DELAY)) {
        lv_refr_now(NULL);
        bsp_lvgl_unlock();
    }
    bsp_backlight_on();
}
