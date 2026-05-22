/**
 * ui_bar_gauges.c — 4 horizontal bar gauges, right cluster.
 *
 * Each gauge: lv_bar, LV_ANIM_OFF, label+value ABOVE the bar (never inside).
 * Native 480×480. Bars stacked vertically in lower 3/4 of display.
 *
 * Channels and ranges:
 *   Boost   0–35 PSI     CYAN < 14 / ORANGE < 20 / RED ≥ 20
 *   ECT   100–300 °F     ramp: CYAN < 150 / WHITE < 200 / ORANGE < 220 / RED ≥ 220
 *   IGN     0–45 °BTDC   GREEN 12–32 / ORANGE edge / RED out of range
 *   IAT    30–200 °F     GREEN < 100 / YELLOW < 130 / ORANGE < 150 / RED ≥ 150
 */
#include "ui_bar_gauges.h"
#include "right-colors.h"

LV_FONT_DECLARE(racehead_12);

typedef struct {
    lv_obj_t   *bar;
    lv_obj_t   *lbl;    /* channel name + value, above bar */
    const char *name;
    float       rmin, rmax;
} bar_gauge_t;

static bar_gauge_t s_gauges[4];

static const struct { const char *name; float rmin, rmax; } GAUGE_DEFS[4] = {
    { "BOOST",  0.0f, 35.0f },
    { "ECT",  100.0f, 300.0f },
    { "IGN",    0.0f, 45.0f },
    { "IAT",   30.0f, 200.0f },
};

void ui_bar_gauges_create(lv_obj_t *parent)
{
    int start_y = 240;
    int bar_h   = 20;
    int bar_w   = 380;
    int row_h   = 52;

    for (int i = 0; i < 4; i++) {
        s_gauges[i].name = GAUGE_DEFS[i].name;
        s_gauges[i].rmin = GAUGE_DEFS[i].rmin;
        s_gauges[i].rmax = GAUGE_DEFS[i].rmax;

        int y = start_y + i * row_h;

        /* Label + value text above bar */
        s_gauges[i].lbl = lv_label_create(parent);
        lv_label_set_text_fmt(s_gauges[i].lbl, "%s  --", GAUGE_DEFS[i].name);
        lv_obj_set_style_text_color(s_gauges[i].lbl, COLOR_WHITE, 0);
        lv_obj_set_style_text_font(s_gauges[i].lbl, &racehead_12, 0);
        lv_obj_set_pos(s_gauges[i].lbl, (480 - bar_w) / 2, y);

        /* Bar */
        s_gauges[i].bar = lv_bar_create(parent);
        lv_obj_set_size(s_gauges[i].bar, bar_w, bar_h);
        lv_obj_set_pos(s_gauges[i].bar, (480 - bar_w) / 2, y + 16);
        lv_bar_set_range(s_gauges[i].bar, 0, 100);
        lv_bar_set_value(s_gauges[i].bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_gauges[i].bar, COLOR_INACTIVE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_gauges[i].bar, COLOR_CYAN, LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_gauges[i].bar, 3, LV_PART_MAIN);
        lv_obj_set_style_radius(s_gauges[i].bar, 3, LV_PART_INDICATOR);
    }
}

static void update_bar(bar_gauge_t *g, float val, lv_color_t col, const char *unit)
{
    float pct = (val - g->rmin) / (g->rmax - g->rmin);
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    lv_bar_set_value(g->bar, (int)(pct * 100), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g->bar, col, LV_PART_INDICATOR);
    lv_label_set_text_fmt(g->lbl, "%s  %.1f %s", g->name, (double)val, unit);
}

void ui_bar_gauges_update(const dash_data_t *d)
{
    update_bar(&s_gauges[0], d->boost,        boost_color(d->boost),        "PSI");
    update_bar(&s_gauges[1], d->coolant_temp, ect_color(d->coolant_temp),   "°F");
    update_bar(&s_gauges[2], d->ign_adv,      ign_color(d->ign_adv),        "°");
    update_bar(&s_gauges[3], d->iat,          iat_color(d->iat),            "°F");
}
