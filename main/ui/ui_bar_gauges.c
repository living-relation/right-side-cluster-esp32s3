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
 *   Boost   0–30 PSI     bar ramp CYAN/ORANGE/RED   · label+value always CYAN
 *   ECT   100–300 °F     bar ramp CYAN/WHITE/ORANGE/RED · label+value always CYAN
 *   IGN     0–45 °BTDC   bar ramp GREEN/ORANGE/RED  · label+value always GREEN
 *   IAT    30–200 °F     bar ramp GREEN/YELLOW/ORANGE/RED · label+value always GREEN
 */
#include "ui_bar_gauges.h"
#include "right-colors.h"

LV_FONT_DECLARE(racehead_19);
LV_FONT_DECLARE(aerospace_22);

typedef struct {
    lv_obj_t   *bar;
    lv_obj_t   *name_lbl;   /* channel name, left, above bar */
    lv_obj_t   *val_lbl;    /* value, right-aligned to bar end, above bar */
    const char *name;
    float       rmin, rmax;
} bar_gauge_t;

static bar_gauge_t s_gauges[4];

static const struct { const char *name; float rmin, rmax; } GAUGE_DEFS[4] = {
    { "BOOST",  0.0f, 30.0f },
    { "ECT",  100.0f, 300.0f },
    { "IGN",    0.0f, 45.0f },
    { "IAT",   30.0f, 200.0f },
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

        int y = start_y + i * row_h;

        /* Channel name + value text — fixed per-channel color (bar fill keeps its ramp):
         * Boost + ECT → cyan; IGN + IAT → green. */
        lv_color_t txt_col = (i <= 1) ? COLOR_CYAN : COLOR_GREEN;
        s_gauges[i].name_lbl = lv_label_create(parent);
        lv_label_set_text(s_gauges[i].name_lbl, GAUGE_DEFS[i].name);
        lv_obj_set_style_text_color(s_gauges[i].name_lbl, txt_col, 0);
        lv_obj_set_style_text_font(s_gauges[i].name_lbl, &racehead_19, 0);
        lv_obj_set_pos(s_gauges[i].name_lbl, bx, y);

        /* Value — right-aligned to the bar's right end, above the bar */
        s_gauges[i].val_lbl = lv_label_create(parent);
        lv_label_set_text(s_gauges[i].val_lbl, "--");
        lv_obj_set_style_text_color(s_gauges[i].val_lbl, txt_col, 0);
        lv_obj_set_style_text_font(s_gauges[i].val_lbl, &aerospace_22, 0);
        lv_obj_set_width(s_gauges[i].val_lbl, bar_w);
        lv_obj_set_style_text_align(s_gauges[i].val_lbl, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_pos(s_gauges[i].val_lbl, bx, y);

        /* Bar */
        s_gauges[i].bar = lv_bar_create(parent);
        lv_obj_set_size(s_gauges[i].bar, bar_w, bar_h);
        lv_obj_set_pos(s_gauges[i].bar, bx, y + 18);
        lv_bar_set_range(s_gauges[i].bar, 0, 100);
        lv_bar_set_value(s_gauges[i].bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_gauges[i].bar, COLOR_INACTIVE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_gauges[i].bar, COLOR_CYAN, LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_gauges[i].bar, 3, LV_PART_MAIN);
        lv_obj_set_style_radius(s_gauges[i].bar, 3, LV_PART_INDICATOR);
        /* No inset — indicator fills flush to the outline at 100 %% */
        lv_obj_set_style_pad_all(s_gauges[i].bar, 0, LV_PART_MAIN);
    }
}

static void update_bar(bar_gauge_t *g, float val, lv_color_t col, const char *unit)
{
    float pct = (val - g->rmin) / (g->rmax - g->rmin);
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    lv_bar_set_value(g->bar, (int)(pct * 100), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g->bar, col, LV_PART_INDICATOR);
    lv_label_set_text_fmt(g->val_lbl, "%.1f %s", (double)val, unit);
}

void ui_bar_gauges_update(const dash_data_t *d)
{
    update_bar(&s_gauges[0], d->boost,        boost_color(d->boost),        "PSI");
    update_bar(&s_gauges[1], d->coolant_temp, ect_color(d->coolant_temp),   "\u00b0F");
    update_bar(&s_gauges[2], d->ign_adv,      ign_color(d->ign_adv),        "\u00b0");
    update_bar(&s_gauges[3], d->iat,          iat_color(d->iat),            "\u00b0F");
}
