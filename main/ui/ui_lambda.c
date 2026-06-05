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
#include "ui_numeric.h"
#include "ui_signal_filter.h"
#include "right-colors.h"
#include <string.h>

LV_FONT_DECLARE(aerospace_56);
LV_IMAGE_DECLARE(lambda_64);

static lv_obj_t *s_glyph = NULL;
static lv_obj_t *s_value = NULL;
static char s_lambda_text[16];

void ui_lambda_create(lv_obj_t *parent)
{
    /* λ glyph — 64×64 ARGB image, tinted via recolor */
    s_glyph = lv_image_create(parent);
    lv_image_set_src(s_glyph, &lambda_64);
    /* Uniform scale (LV_SCALE_NONE=256); 336 ≈ 131 % for a bolder λ asset */
    lv_image_set_scale(s_glyph, 336);
    lv_obj_set_style_image_recolor(s_glyph, COLOR_RED_HOT, 0);
    lv_obj_set_style_image_recolor_opa(s_glyph, LV_OPA_COVER, 0);
    lv_obj_align(s_glyph, LV_ALIGN_TOP_MID, -120, 73);

    /* Numeric value — Aerospace 56 px (0 = no UART data yet, matches first live paint) */
    s_value = lv_label_create(parent);
    lv_label_set_text(s_value, "0.000");
    lv_obj_set_style_text_color(s_value, COLOR_RED_HOT, 0);
    lv_obj_set_style_text_font(s_value, &aerospace_56, 0);
    lv_obj_set_style_text_align(s_value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_value, LV_ALIGN_TOP_MID, 16, 96);
    strncpy(s_lambda_text, "0.000", sizeof(s_lambda_text) - 1);
    s_lambda_text[sizeof(s_lambda_text) - 1] = '\0';
}

void ui_lambda_update(const dash_data_t *d)
{
    char buf[16];
    float lambda = ui_filter_lambda(d->lambda);
    lv_color_t col = lambda_color(lambda);
    ui_fmt_3dp(buf, sizeof(buf), lambda);
    if (strcmp(buf, s_lambda_text) != 0) {
        strncpy(s_lambda_text, buf, sizeof(s_lambda_text) - 1);
        s_lambda_text[sizeof(s_lambda_text) - 1] = '\0';
        lv_label_set_text(s_value, s_lambda_text);
    }
    lv_obj_set_style_text_color(s_value, col, 0);
    lv_obj_set_style_image_recolor(s_glyph, col, 0);
    lv_obj_set_style_image_recolor_opa(s_glyph, LV_OPA_COVER, 0);
}
