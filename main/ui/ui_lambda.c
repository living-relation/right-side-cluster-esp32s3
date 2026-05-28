/**
 * ui_lambda.c — Lambda digital block, right cluster.
 *
 * Layout per center-06-lvgl-widgets.md §"Lambda specifically":
 *   Native 480×480. Both labels TOP_MID anchored.
 *   value:    LV_ALIGN_TOP_MID, x_off = +16,  y_off = 96  (Aerospace 56 px)
 *   λ glyph:  lv_obj_align_to(glyph, value, LV_ALIGN_OUT_LEFT_MID, -8, 0)
 *   Both LV_TEXT_ALIGN_CENTER. Do NOT split at the decimal.
 *
 * Color per lambda_color() ramp (right-colors.h):
 *   < 0.70           RED
 *   0.70 – 0.749     ORANGE
 *   0.75 – 1.019     GREEN
 *   1.02 – 1.049     ORANGE
 *   >= 1.05          RED
 *
 * Never displayed as AFR.
 */
#include "ui_lambda.h"
#include "right-colors.h"

LV_FONT_DECLARE(aerospace_56);
LV_FONT_DECLARE(lambda_56);

static lv_obj_t *s_glyph = NULL;
static lv_obj_t *s_value = NULL;

/* Lambda stored ×1000 as int: 0.500 → 500, 2.000 → 2000 */
static int m_lambda_current = 1000;
static int m_lambda_target  = 1000;

static void lambda_smooth_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (m_lambda_current == m_lambda_target) return;
    int delta = m_lambda_target - m_lambda_current;
    int step  = 2;
    if (delta > step || delta < -step)
        m_lambda_current += (delta > 0) ? step : -step;
    else
        m_lambda_current = m_lambda_target;
    float lam = (float)m_lambda_current / 1000.0f;
    lv_color_t col = lambda_color(lam);
    lv_label_set_text_fmt(s_value, "%.3f", (double)lam);
    lv_obj_set_style_text_color(s_value, col, 0);
    lv_obj_set_style_text_color(s_glyph, col, 0);
}

void ui_lambda_create(lv_obj_t *parent)
{
    /* Numeric value — Aerospace 56 px; anchored first so glyph can align to it */
    s_value = lv_label_create(parent);
    lv_label_set_text(s_value, "1.000");
    lv_obj_set_style_text_color(s_value, COLOR_GREEN, 0);
    lv_obj_set_style_text_font(s_value, &aerospace_56, 0);
    lv_obj_set_style_text_align(s_value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_value, LV_ALIGN_TOP_MID, 16, 96);

    /* λ glyph — Arial Bold 56 px (λ U+03BB is not in the custom Racehead/Aerospace sets;
     * Montserrat built-in tops at 48 px). Bold weight matches the visual mass of the
     * adjacent Aerospace 56 px numeric value. */
    s_glyph = lv_label_create(parent);
    lv_label_set_text(s_glyph, "\xCE\xBB");   /* UTF-8 λ */
    lv_obj_set_style_text_color(s_glyph, COLOR_GREEN, 0);
    lv_obj_set_style_text_font(s_glyph, &lambda_56, 0);
    lv_obj_set_style_text_align(s_glyph, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(s_glyph, s_value, LV_ALIGN_OUT_LEFT_MID, -8, 0);

    lv_timer_create(lambda_smooth_timer_cb, 50, NULL);
}

void ui_lambda_update(const dash_data_t *d)
{
    int target = (int)(d->lambda * 1000.0f);
    if (target < 0)    target = 0;
    if (target > 3000) target = 3000;
    m_lambda_target = target;
}
