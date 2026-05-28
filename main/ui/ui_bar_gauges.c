/**
 * ui_bar_gauges.c — 4 horizontal bar gauges, right cluster.
 *
 * Each gauge: lv_bar, LV_ANIM_OFF; name label left-aligned, value label
 * right-aligned, both above the bar and colored to match the bar fill.
 * Native 480×480. Bars stacked vertically; stack starts at y=150, row_h=62.
 *
 * Channels and ranges:
 *   Boost   0–35 PSI     CYAN < 14 / ORANGE < 20 / RED ≥ 20
 *   ECT   100–300 °F     ramp: CYAN < 150 / WHITE < 200 / ORANGE < 220 / RED ≥ 220
 *   IGN     0–45 °BTDC   GREEN 12–32 / ORANGE edge / RED out of range
 *   IAT    30–200 °F     GREEN < 100 / YELLOW < 130 / ORANGE < 150 / RED ≥ 150
 */
#include "ui_bar_gauges.h"
#include "right-colors.h"

LV_FONT_DECLARE(racehead_28);

typedef struct {
    lv_obj_t   *bar;
    lv_obj_t   *name_lbl;  /* static channel name, left-aligned */
    lv_obj_t   *val_lbl;   /* live value+unit, right-aligned */
    const char *name;
    float       rmin, rmax;
} bar_gauge_t;

static bar_gauge_t s_gauges[4];

/* Boost and IGN smoothing: stored ×10 as int for 0.1-unit precision */
static int m_boost_current = 0;
static int m_boost_target  = 0;
static int m_ign_current   = 0;
static int m_ign_target    = 0;

static const struct { const char *name; float rmin, rmax; } GAUGE_DEFS[4] = {
    { "BOOST",  0.0f, 35.0f },
    { "ECT",  100.0f, 300.0f },
    { "IGN",    0.0f, 45.0f },
    { "IAT",   30.0f, 200.0f },
};

static void update_bar(bar_gauge_t *g, float val, lv_color_t col, const char *unit)
{
    float pct = (val - g->rmin) / (g->rmax - g->rmin);
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    lv_bar_set_value(g->bar, (int)(pct * 100), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g->bar, col, LV_PART_INDICATOR);
    lv_label_set_text_fmt(g->val_lbl, "%.1f %s", (double)val, unit);
    lv_obj_set_style_text_color(g->val_lbl,  col, 0);
    lv_obj_set_style_text_color(g->name_lbl, col, 0);
}

static void boost_smooth_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (m_boost_current == m_boost_target) return;
    int delta = m_boost_target - m_boost_current;
    int step  = 1;
    if (delta > step || delta < -step)
        m_boost_current += (delta > 0) ? step : -step;
    else
        m_boost_current = m_boost_target;
    float v = (float)m_boost_current / 10.0f;
    update_bar(&s_gauges[0], v, boost_color(v), "PSI");
}

static void ign_smooth_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (m_ign_current == m_ign_target) return;
    int delta = m_ign_target - m_ign_current;
    int step  = 1;
    if (delta > step || delta < -step)
        m_ign_current += (delta > 0) ? step : -step;
    else
        m_ign_current = m_ign_target;
    float v = (float)m_ign_current / 10.0f;
    update_bar(&s_gauges[2], v, ign_color(v), "\xC2\xB0");
}

void ui_bar_gauges_create(lv_obj_t *parent)
{
    int start_y  = 150;
    int bar_h    = 20;
    int bar_w    = 290;
    int row_h    = 62;
    int bar_left = (480 - bar_w) / 2;   /* 95 */

    for (int i = 0; i < 4; i++) {
        s_gauges[i].name = GAUGE_DEFS[i].name;
        s_gauges[i].rmin = GAUGE_DEFS[i].rmin;
        s_gauges[i].rmax = GAUGE_DEFS[i].rmax;

        int y = start_y + i * row_h;

        /* Channel name — left edge of bar, color tracks bar fill */
        s_gauges[i].name_lbl = lv_label_create(parent);
        lv_label_set_text(s_gauges[i].name_lbl, GAUGE_DEFS[i].name);
        lv_obj_set_style_text_color(s_gauges[i].name_lbl, COLOR_WHITE, 0);
        lv_obj_set_style_text_font(s_gauges[i].name_lbl, &racehead_28, 0);
        lv_obj_set_pos(s_gauges[i].name_lbl, bar_left, y);

        /* Live value — right-aligned to bar right edge, same color as bar fill */
        s_gauges[i].val_lbl = lv_label_create(parent);
        lv_label_set_text(s_gauges[i].val_lbl, "--");
        lv_obj_set_style_text_color(s_gauges[i].val_lbl, COLOR_WHITE, 0);
        lv_obj_set_style_text_font(s_gauges[i].val_lbl, &racehead_28, 0);
        lv_obj_set_width(s_gauges[i].val_lbl, bar_w);
        lv_obj_set_style_text_align(s_gauges[i].val_lbl, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_pos(s_gauges[i].val_lbl, bar_left, y);

        /* Bar */
        s_gauges[i].bar = lv_bar_create(parent);
        lv_obj_set_size(s_gauges[i].bar, bar_w, bar_h);
        lv_obj_set_pos(s_gauges[i].bar, bar_left, y + 30);
        lv_bar_set_range(s_gauges[i].bar, 0, 100);
        lv_bar_set_value(s_gauges[i].bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_gauges[i].bar, COLOR_INACTIVE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_gauges[i].bar, COLOR_CYAN, LV_PART_INDICATOR);
        lv_obj_set_style_radius(s_gauges[i].bar, 3, LV_PART_MAIN);
        lv_obj_set_style_radius(s_gauges[i].bar, 3, LV_PART_INDICATOR);
    }

    lv_timer_create(boost_smooth_timer_cb, 50, NULL);
    lv_timer_create(ign_smooth_timer_cb,   50, NULL);
}

void ui_bar_gauges_update(const dash_data_t *d)
{
    m_boost_target = (int)(d->boost   * 10.0f);
    m_ign_target   = (int)(d->ign_adv * 10.0f);
    update_bar(&s_gauges[1], d->coolant_temp, ect_color(d->coolant_temp),   "\xC2\xB0""F");
    update_bar(&s_gauges[3], d->iat,          iat_color(d->iat),            "\xC2\xB0""F");
}
