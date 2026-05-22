/**
 * ui_lambda.c — Lambda digital block, right cluster.
 *
 * Layout per center-06-lvgl-widgets.md §"Lambda specifically":
 *   Native 480×480. Both labels TOP_MID anchored.
 *   λ glyph:  LV_ALIGN_TOP_MID, x_off = -124, y_off = 96  (serif 64 px)
 *   value:    LV_ALIGN_TOP_MID, x_off = +16,  y_off = 96  (Aerospace 56 px)
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

static lv_obj_t *s_glyph = NULL;
static lv_obj_t *s_value = NULL;

void ui_lambda_create(lv_obj_t *parent)
{
    /* λ glyph — serif 64 px; stub uses montserrat until real font is converted */
    s_glyph = lv_label_create(parent);
    lv_label_set_text(s_glyph, "\xCE\xBB");   /* UTF-8 λ */
    lv_obj_set_style_text_color(s_glyph, COLOR_GREEN, 0);
    lv_obj_set_style_text_font(s_glyph, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_align(s_glyph, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_glyph, LV_ALIGN_TOP_MID, -124, 96);

    /* Numeric value — Aerospace 56 px */
    s_value = lv_label_create(parent);
    lv_label_set_text(s_value, "1.000");
    lv_obj_set_style_text_color(s_value, COLOR_GREEN, 0);
    lv_obj_set_style_text_font(s_value, &aerospace_56, 0);
    lv_obj_set_style_text_align(s_value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_value, LV_ALIGN_TOP_MID, 16, 96);
}

void ui_lambda_update(const dash_data_t *d)
{
    lv_color_t col = lambda_color(d->lambda);
    lv_label_set_text_fmt(s_value, "%.3f", (double)d->lambda);
    lv_obj_set_style_text_color(s_value, col, 0);
    lv_obj_set_style_text_color(s_glyph, col, 0);
}
