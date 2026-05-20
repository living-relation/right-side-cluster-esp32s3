/**
 * ui_Screen1.c — TrackCluster RIGHT cluster (480×480 round)
 *
 * Widgets only. Updates happen in paint_timer_cb (main.c).
 * AFR is BANNED — lambda only (float, 0.70..1.05).
 * Bar value labels are RIGHT-anchored.
 */

#include "../ui.h"
#include "lvgl.h"
#include "../../right-colors.h"
#include "../../right-dash_data.h"

#define CX 240
#define CY 240
#define BX 120
#define BW 240
#define BH 20

LV_FONT_DECLARE(aerospace_22);
LV_FONT_DECLARE(aerospace_56);

lv_obj_t *ui_Screen1            = NULL;

/* Lambda band */
lv_obj_t *ui_lambda_band        = NULL;
lv_obj_t *ui_img_lambda         = NULL;   /* placeholder label until lambda_64.c arrives */
lv_obj_t *ui_label_lambda_val   = NULL;

/* Boost bar */
lv_obj_t *ui_bar_boost          = NULL;
lv_obj_t *ui_label_boost_ch     = NULL;
lv_obj_t *ui_label_boost_val    = NULL;

/* ECT bar */
lv_obj_t *ui_bar_ect            = NULL;
lv_obj_t *ui_label_ect_ch       = NULL;
lv_obj_t *ui_label_ect_val      = NULL;

/* IGN bar */
lv_obj_t *ui_bar_ign            = NULL;
lv_obj_t *ui_label_ign_ch       = NULL;
lv_obj_t *ui_label_ign_val      = NULL;

/* IAT bar */
lv_obj_t *ui_bar_iat            = NULL;
lv_obj_t *ui_label_iat_ch       = NULL;
lv_obj_t *ui_label_iat_val      = NULL;

/* No-ECU pill */
lv_obj_t *ui_no_ecu_pill        = NULL;
lv_obj_t *ui_no_ecu_label       = NULL;

static lv_obj_t *make_bar(lv_obj_t *parent, int x, int y, int w, int h,
                          int min, int max)
{
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_set_size(bar, w, h);
    lv_obj_set_pos(bar, x, y);
    lv_bar_set_range(bar, min, max);
    lv_bar_set_value(bar, min, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, COLOR_BG_SECONDARY, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar, COLOR_GREEN, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    return bar;
}

static lv_obj_t *make_ch_label(lv_obj_t *parent, int x, int y, const char *text)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_pos(l, x, y);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, COLOR_GREY, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(l, &aerospace_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    return l;
}

static lv_obj_t *make_val_label(lv_obj_t *parent, int x_right, int y)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_width(l, 80);
    lv_obj_set_pos(l, x_right - 80, y);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(l, "0");
    lv_obj_set_style_text_color(l, COLOR_WHITE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(l, &aerospace_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    return l;
}

void ui_Screen1_screen_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen1, COLOR_BG_PRIMARY,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen1, LV_OPA_COVER,
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Lambda band — full-width tinted strip across the top */
    ui_lambda_band = lv_obj_create(ui_Screen1);
    lv_obj_set_size(ui_lambda_band, 480, 120);
    lv_obj_set_pos(ui_lambda_band, 0, 0);
    lv_obj_set_style_radius(ui_lambda_band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_lambda_band, COLOR_GREEN,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_lambda_band, 60, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_lambda_band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_lambda_band, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui_lambda_band, LV_OBJ_FLAG_CLICKABLE);

    /* Lambda symbol — TODO: replace with lv_image once lambda_64.c arrives */
    ui_img_lambda = lv_label_create(ui_Screen1);
    lv_obj_set_pos(ui_img_lambda, 116, 96);
    lv_label_set_text(ui_img_lambda, "L");  /* lambda placeholder */
    lv_obj_set_style_text_color(ui_img_lambda, COLOR_WHITE,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_img_lambda, &aerospace_56,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_label_lambda_val = lv_label_create(ui_Screen1);
    lv_obj_set_pos(ui_label_lambda_val, 220, 96);
    lv_label_set_text(ui_label_lambda_val, "0.000");
    lv_obj_set_style_text_color(ui_label_lambda_val, COLOR_GREEN,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_label_lambda_val, &aerospace_56,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Boost: range 0..35 PSI */
    ui_bar_boost      = make_bar(ui_Screen1, BX, 204, BW, BH, 0, 35);
    ui_label_boost_ch = make_ch_label(ui_Screen1, BX,        198, "BOOST PSI");
    ui_label_boost_val= make_val_label(ui_Screen1, BX + BW,  198);

    /* ECT: range 100..300 °F */
    ui_bar_ect      = make_bar(ui_Screen1, BX, 268, BW, BH, 100, 300);
    ui_label_ect_ch = make_ch_label(ui_Screen1, BX,        262, "ECT F");
    ui_label_ect_val= make_val_label(ui_Screen1, BX + BW,  262);

    /* IGN: range 0..45 deg */
    ui_bar_ign      = make_bar(ui_Screen1, BX, 332, BW, BH, 0, 45);
    ui_label_ign_ch = make_ch_label(ui_Screen1, BX,        326, "IGN");
    ui_label_ign_val= make_val_label(ui_Screen1, BX + BW,  326);

    /* IAT: range 30..200 °F */
    ui_bar_iat      = make_bar(ui_Screen1, BX, 396, BW, BH, 30, 200);
    ui_label_iat_ch = make_ch_label(ui_Screen1, BX,        390, "IAT F");
    ui_label_iat_val= make_val_label(ui_Screen1, BX + BW,  390);

    /* No-ECU pill, centered, hidden by default */
    ui_no_ecu_pill = lv_obj_create(ui_Screen1);
    lv_obj_set_size(ui_no_ecu_pill, 300, 50);
    lv_obj_set_align(ui_no_ecu_pill, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_no_ecu_pill, 25, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_no_ecu_pill, COLOR_BG_SECONDARY,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_no_ecu_pill, LV_OPA_COVER,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_no_ecu_pill, COLOR_ORANGE,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_no_ecu_pill, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_no_ecu_pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_no_ecu_pill, LV_OBJ_FLAG_HIDDEN);

    ui_no_ecu_label = lv_label_create(ui_no_ecu_pill);
    lv_obj_set_align(ui_no_ecu_label, LV_ALIGN_CENTER);
    lv_label_set_text(ui_no_ecu_label, "WAITING FOR ECU");
    lv_obj_set_style_text_color(ui_no_ecu_label, COLOR_WHITE,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_no_ecu_label, &aerospace_22,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_Screen1_screen_destroy(void)
{
    if (ui_Screen1) lv_obj_del(ui_Screen1);
    ui_Screen1 = NULL;
    ui_lambda_band = NULL;
    ui_img_lambda = NULL;
    ui_label_lambda_val = NULL;
    ui_bar_boost = NULL; ui_label_boost_ch = NULL; ui_label_boost_val = NULL;
    ui_bar_ect = NULL;   ui_label_ect_ch = NULL;   ui_label_ect_val = NULL;
    ui_bar_ign = NULL;   ui_label_ign_ch = NULL;   ui_label_ign_val = NULL;
    ui_bar_iat = NULL;   ui_label_iat_ch = NULL;   ui_label_iat_val = NULL;
    ui_no_ecu_pill = NULL; ui_no_ecu_label = NULL;
}
