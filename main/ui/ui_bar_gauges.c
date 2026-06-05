/**
 * ui_bar_gauges.c — 4 horizontal bar gauges, right cluster.
 *
 * Each gauge: lv_bar, LV_ANIM_OFF. Channel name left-aligned, value right-aligned
 * to the bar's right end, both above the bar (never inside).
 * Native 480×480. Bars stacked vertically, shifted up so the set clears the bezel.
 *
 * Geometry: bar_w 360, bar_h 20, x=(480-360)/2=60, start_y 182, row pitch 58.
 *
 * Channels and ranges:
 *   Boost   0–30 PSI     bar CYAN, RED ≥25 PSI       · label white, value white
 *   ECT   100–300 °F     bar ramp CYAN/ORANGE/RED (200 / 230 °F thresholds)
 *   IGN     0–45 °BTDC   bar ramp GREEN/ORANGE/RED  · label+value always GREEN
 *   IAT    30–200 °F     bar ramp GREEN/YELLOW/ORANGE/RED · label+value always GREEN
 */
#include "ui_bar_gauges.h"
#include "ui_numeric.h"
#include "ui_signal_filter.h"
#include "ui_debug_log.h"
#include "right-colors.h"
#include <string.h>

LV_FONT_DECLARE(racehead_22);
LV_FONT_DECLARE(aerospace_22);

typedef struct {
    lv_obj_t   *bar;
    lv_obj_t   *name_lbl;   /* channel name, left, above bar */
    lv_obj_t   *val_lbl;    /* value, right-aligned to bar end, above bar */
    const char *name;
    float       rmin, rmax;
} bar_gauge_t;

static bar_gauge_t s_gauges[4];
static char s_val_text[4][16];
static uint32_t s_dbg_bar_updates;

static const struct { const char *name; const char *unit; float rmin, rmax; } GAUGE_DEFS[4] = {
    { "BOOST PSI",  "",   0.0f, 30.0f },
    { "ECT",        "F", 100.0f, 300.0f },
    { "IGN",        "",   0.0f, 45.0f },
    { "IAT",        "F",  30.0f, 200.0f },
};

void ui_bar_gauges_create(lv_obj_t *parent)
{
    int start_y = 182;     /* shifted up (3× the added row spacing) */
    int bar_h   = 20;
    int bar_w   = 320;     /* shortened a further 20 px */
    int row_h   = 58;      /* spaced wider */
    int bx      = (480 - bar_w) / 2;   /* 80 */

    for (int i = 0; i < 4; i++) {
        s_gauges[i].name = GAUGE_DEFS[i].name;
        s_gauges[i].rmin = GAUGE_DEFS[i].rmin;
        s_gauges[i].rmax = GAUGE_DEFS[i].rmax;

        int y_ofs = (i == 0) ? -7 : 0;
        int y = start_y + i * row_h + y_ofs;

        s_gauges[i].name_lbl = lv_label_create(parent);
        lv_label_set_text(s_gauges[i].name_lbl, GAUGE_DEFS[i].name);
        lv_obj_set_style_text_color(s_gauges[i].name_lbl, COLOR_WHITE, 0);
        lv_obj_set_style_text_font(s_gauges[i].name_lbl, &racehead_22, 0);
        lv_obj_set_pos(s_gauges[i].name_lbl, bx, y);
        // #region agent log
        if (i == 0) {
            UI_DBG_LOG("G", "ui_bar_gauges.c:create", "label_font",
                       "\"font_h\":%d,\"line_h\":%d",
                       (int)racehead_22.line_height, (int)racehead_22.line_height);
        }
        // #endregion

        /* Value — right-aligned to the bar's right end, above the bar */
        s_gauges[i].val_lbl = lv_label_create(parent);
        lv_label_set_text(s_gauges[i].val_lbl, "0.0");
        lv_obj_set_style_text_color(s_gauges[i].val_lbl, COLOR_CYAN, 0);
        lv_obj_set_style_text_font(s_gauges[i].val_lbl, &aerospace_22, 0);
        lv_obj_set_width(s_gauges[i].val_lbl, bar_w);
        lv_obj_set_style_text_align(s_gauges[i].val_lbl, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_pos(s_gauges[i].val_lbl, bx, (i == 0) ? (y - 6) : (y - 4));

        /* Bar — boost row is taller with a white frame */
        int row_bar_h = (i == 0) ? (bar_h + 8) : bar_h;
        s_gauges[i].bar = lv_bar_create(parent);
        lv_obj_set_size(s_gauges[i].bar, bar_w, row_bar_h);
        lv_obj_set_pos(s_gauges[i].bar, bx, y + 18);
        lv_bar_set_range(s_gauges[i].bar, 0, 100);
        lv_bar_set_value(s_gauges[i].bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_gauges[i].bar, COLOR_INACTIVE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_gauges[i].bar, COLOR_CYAN, LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_gauges[i].bar, 3, LV_PART_MAIN);
        lv_obj_set_style_radius(s_gauges[i].bar, 2, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(s_gauges[i].bar, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(s_gauges[i].bar, LV_OPA_COVER, LV_PART_INDICATOR);
        if (i == 0) {
            /* Inset fill so 3 px white border on MAIN stays outside the indicator */
            lv_obj_set_style_pad_all(s_gauges[i].bar, 3, LV_PART_MAIN);
            lv_obj_set_style_border_width(s_gauges[i].bar, 3, LV_PART_MAIN);
            lv_obj_set_style_border_color(s_gauges[i].bar, COLOR_WHITE, LV_PART_MAIN);
            lv_obj_set_style_border_opa(s_gauges[i].bar, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_width(s_gauges[i].bar, 0, LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_pad_all(s_gauges[i].bar, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(s_gauges[i].bar, 1, LV_PART_MAIN);
            lv_obj_set_style_border_color(s_gauges[i].bar, COLOR_GREY_DARK, LV_PART_MAIN);
            lv_obj_set_style_border_opa(s_gauges[i].bar, LV_OPA_COVER, LV_PART_MAIN);
        }
    }
}

static void update_bar(bar_gauge_t *g, float val, lv_color_t bar_col, lv_color_t text_col)
{
    float pct = (val - g->rmin) / (g->rmax - g->rmin);
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    lv_bar_set_value(g->bar, (int)(pct * 100), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g->bar, bar_col, LV_PART_INDICATOR);

    char buf[16];
    int idx = (int)(g - s_gauges);
    ui_fmt_1dp(buf, sizeof(buf), val);
    if (idx >= 0 && idx < 4 && strcmp(buf, s_val_text[idx]) != 0) {
        strncpy(s_val_text[idx], buf, sizeof(s_val_text[idx]) - 1);
        s_val_text[idx][sizeof(s_val_text[idx]) - 1] = '\0';
        lv_label_set_text(g->val_lbl, s_val_text[idx]);
    }
    lv_obj_set_style_text_color(g->val_lbl, text_col, 0);
    // #region agent log
    if (s_dbg_bar_updates < 3 && g == &s_gauges[0]) {
        UI_DBG_LOG("A", "ui_bar_gauges.c:update_bar", "bar0_fmt",
                   "\"buf\":\"%s\",\"val\":%d,\"pct\":%d",
                   buf, (int)val, (int)(pct * 100));
        s_dbg_bar_updates++;
    }
    // #endregion
}

void ui_bar_gauges_update(const dash_data_t *d)
{
    float boost = ui_filter_boost(d->boost);
    float ign   = ui_filter_ign(d->ign_adv);
    update_bar(&s_gauges[0], boost, boost_color(boost), COLOR_WHITE);
    update_bar(&s_gauges[1], d->coolant_temp, ect_color(d->coolant_temp),
               ect_color(d->coolant_temp));
    update_bar(&s_gauges[2], ign, ign_color(ign), ign_color(ign));
    update_bar(&s_gauges[3], d->iat, iat_color(d->iat), iat_color(d->iat));
}
