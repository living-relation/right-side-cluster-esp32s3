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
#include "ui_lambda.h"
#include "ui_bar_gauges.h"
#include "ui_menu_popup.h"
#include "ui_warning.h"
#include "ui_boot.h"
#include "dash_data.h"
#include "right-colors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"

extern portMUX_TYPE g_dash_mux;
static bool s_live = false;

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
    ui_menu_popup_update(&snap);
    ui_warning_update(&snap);
}

void ui_on_boot_complete(void)
{
    s_live = true;
    lv_timer_create(ui_paint_tick, 33, NULL);
}

void ui_init(lv_disp_t *disp)
{
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_set_style_bg_color(scr, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    ui_lambda_create(scr);
    ui_bar_gauges_create(scr);
    ui_menu_popup_create(scr);
    ui_warning_create(scr);
    ui_boot_start(scr, ui_on_boot_complete);
}
