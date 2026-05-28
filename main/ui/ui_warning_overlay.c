/**
 * ui_warning_overlay.c — engine-protect critical warning overlay, right cluster.
 *
 * Activates on any of: DASH_FLAG_KNOCK, DASH_FLAG_BOOST_CUT, DASH_FLAG_FUEL_CUT,
 * DASH_FLAG_IGN_CUT, DASH_FLAG_TRACTION_CUT, DASH_FLAG_REV_LIMIT.
 *
 * Priority (highest shown when multiple flags active):
 *   KNOCK > BOOST_CUT > FUEL_CUT > IGN_CUT > TRACTION_CUT > REV_LIMIT
 *
 * Blinks at 2 Hz (250 ms on / 250 ms off) for peripheral-vision visibility.
 * Does NOT cover COOLANT_HOT / LAMBDA_BAD / OVERBOOST — those use gauge colors.
 */
#include "ui_warning_overlay.h"
#include "right-colors.h"

LV_FONT_DECLARE(racehead_28);

static lv_obj_t *s_overlay = NULL;
static lv_obj_t *s_title   = NULL;
static lv_obj_t *s_sub     = NULL;
static bool      s_blink_on = false;

static void blink_timer_cb(lv_timer_t *t)
{
    (void)t;
    s_blink_on = !s_blink_on;
}

void ui_warning_overlay_create(lv_obj_t *parent)
{
    s_overlay = lv_obj_create(parent);
    lv_obj_set_size(s_overlay, 480, 480);
    lv_obj_center(s_overlay);
    lv_obj_set_style_bg_color(s_overlay, COLOR_GOLD, 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_overlay, 0, 0);
    lv_obj_set_style_radius(s_overlay, 0, 0);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_overlay);

    s_title = lv_label_create(s_overlay);
    lv_label_set_text(s_title, "");
    lv_obj_set_style_text_color(s_title, lv_color_black(), 0);
    lv_obj_set_style_text_font(s_title, &racehead_28, 0);
    lv_obj_set_width(s_title, 480);
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_title, LV_ALIGN_CENTER, 0, -20);

    s_sub = lv_label_create(s_overlay);
    lv_label_set_text(s_sub, "");
    lv_obj_set_style_text_color(s_sub, lv_color_black(), 0);
    lv_obj_set_style_text_font(s_sub, &racehead_28, 0);
    lv_obj_set_width(s_sub, 480);
    lv_obj_set_style_text_align(s_sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_sub, LV_ALIGN_CENTER, 0, 20);

    lv_timer_create(blink_timer_cb, 250, NULL);
}

void ui_warning_overlay_update(const dash_data_t *d)
{
    uint16_t f = d->flags;
    const char *title = NULL;
    const char *sub   = NULL;

    if      (f & DASH_FLAG_KNOCK)        { title = "KNOCK";     sub = NULL;              }
    else if (f & DASH_FLAG_BOOST_CUT)    { title = "BOOST CUT"; sub = "ECU LIMITING";    }
    else if (f & DASH_FLAG_FUEL_CUT)     { title = "FUEL CUT";  sub = "ECU ACTIVE";      }
    else if (f & DASH_FLAG_IGN_CUT)      { title = "IGN CUT";   sub = "ECU ACTIVE";      }
    else if (f & DASH_FLAG_TRACTION_CUT) { title = "TC ACTIVE"; sub = "TRACTION CUT";    }
    else if (f & DASH_FLAG_REV_LIMIT)    { title = "REV LIMIT"; sub = "RPM LIMITED";      }

    if (!title) {
        lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_label_set_text(s_title, title);

    /* KNOCK subtitle shows live retard value */
    if (f & DASH_FLAG_KNOCK)
        lv_label_set_text_fmt(s_sub, "%.1f\xC2\xB0 RETARD", (double)d->knock_level);
    else
        lv_label_set_text(s_sub, sub ? sub : "");

    if (s_blink_on)
        lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
}
