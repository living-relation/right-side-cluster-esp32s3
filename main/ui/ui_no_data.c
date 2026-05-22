/* Identical to left cluster — see left-side-cluster-esp32s3/main/ui/ui_no_data.c */
#include "ui_no_data.h"
#include "dash_data.h"

LV_FONT_DECLARE(racehead_14);

static lv_obj_t *s_pill = NULL;
static bool      s_pill_vis = false;
static uint32_t  s_strobe_last = 0;
static bool      s_strobe_hi = true;

void ui_no_data_create(lv_obj_t *parent)
{
    s_pill = lv_obj_create(parent);
    lv_obj_set_size(s_pill, 180, 48);
    lv_obj_align(s_pill, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_pill, lv_color_hex(0xFF1744), 0);
    lv_obj_set_style_bg_opa(s_pill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_pill, 24, 0);
    lv_obj_set_style_border_width(s_pill, 0, 0);
    lv_obj_clear_flag(s_pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *lbl = lv_label_create(s_pill);
    lv_label_set_text(lbl, "NO ECU");
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &racehead_14, 0);
    lv_obj_center(lbl);
    lv_obj_add_flag(s_pill, LV_OBJ_FLAG_HIDDEN);
}

void ui_no_data_update(uint32_t age_ms)
{
    bool want_vis = (age_ms > DASH_STALE_OFF_MS);
    if (want_vis != s_pill_vis) {
        s_pill_vis = want_vis;
        if (want_vis) lv_obj_clear_flag(s_pill, LV_OBJ_FLAG_HIDDEN);
        else          lv_obj_add_flag(s_pill, LV_OBJ_FLAG_HIDDEN);
    }
    if (!want_vis) return;
    uint32_t now = lv_tick_get();
    if ((now - s_strobe_last) >= 600) {
        s_strobe_hi = !s_strobe_hi;
        s_strobe_last = now;
        lv_obj_set_style_opa(s_pill, s_strobe_hi ? LV_OPA_COVER : (lv_opa_t)(0.4f*255), 0);
    }
}
