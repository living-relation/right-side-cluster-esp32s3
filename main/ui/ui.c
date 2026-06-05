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

extern portMUX_TYPE g_dash_mux;
static lv_obj_t *s_screen = NULL;
static bool s_live = false;

/* Overboost full-screen flash: latch ≥28 PSI, release <25 PSI (filtered boost). */
#define BOOST_BG_ON_PSI    28.0f
#define BOOST_BG_OFF_PSI   25.0f
#define BOOST_BG_FLASH_MS  100U

static bool     s_boost_bg_lat;
static bool     s_boost_bg_flash_on;
static uint32_t s_boost_bg_flash_ms;

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

static void ui_live_start_cb(lv_timer_t *t)
{
    lv_timer_delete(t);
    s_live = true;
    lv_timer_create(ui_paint_tick, UI_PAINT_TICK_MS, NULL);
}

void ui_on_boot_complete(void)
{
    // #region agent log
    UI_DBG_LOG("B", "ui.c:ui_on_boot_complete", "boot_done", "\"bench\":%d", (int)CONFIG_TC_BENCH_MODE);
    // #endregion
#if CONFIG_TC_BENCH_MODE
    bench_demo_start();
#endif
    lv_timer_t *t = lv_timer_create(ui_live_start_cb, BOOT_LIVE_SETTLE_MS, NULL);
    lv_timer_set_repeat_count(t, 1);
}

void ui_init(lv_disp_t *disp)
{
    s_screen = lv_disp_get_scr_act(disp);
    lv_obj_set_style_bg_color(s_screen, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);

    ui_lambda_create(s_screen);
    ui_bar_gauges_create(s_screen);
    ui_menu_popup_create(s_screen);
    ui_warning_create(s_screen);
    ui_boot_start(s_screen, ui_on_boot_complete);

    if (bsp_lvgl_lock(portMAX_DELAY)) {
        lv_refr_now(NULL);
        bsp_lvgl_unlock();
    }
    bsp_backlight_on();
}
