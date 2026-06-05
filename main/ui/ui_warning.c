/**
 * ui_warning.c — ECU engine-protection warning overlay, right cluster.
 *
 * Compact RED box pinned to the TOP of the display (no full-screen background) so it
 * coexists with the centered encoder menu popup without overlapping it. Large black
 * text. Triggered by the engine-protection warnings the ECU sends in CAN status frame
 * 0x3EE (decoded on center, forwarded in the RIGHT UART frame as the `warn` bitfield).
 *
 * Shows up to 3 active warnings, highest priority first (see WARN_DEFS order).
 * Widget-represented conditions (low oil pressure, low fuel, high temps) are NOT shown
 * here — they are already communicated by gauge color.
 */
#include "ui_warning.h"
#include "ui_debug_log.h"
#include "right-colors.h"

LV_FONT_DECLARE(racehead_14);
LV_FONT_DECLARE(racehead_17);

#define WARN_MAX_SHOWN 3

/* Priority order: highest engine-damage risk first. */
static const struct { uint8_t bit; const char *name; } WARN_DEFS[] = {
    { DASH_WARN_KNOCK,     "KNOCK"      },
    { DASH_WARN_IGN_CUT,   "IGN CUT"    },
    { DASH_WARN_FUEL_CUT,  "FUEL CUT"   },
    { DASH_WARN_BOOST_CUT, "BOOST CUT"  },
    { DASH_WARN_SENSOR,    "SENSOR ERR" },
    { DASH_WARN_THROTTLE,  "THROTTLE"   },
};
#define WARN_DEFS_N ((int)(sizeof(WARN_DEFS) / sizeof(WARN_DEFS[0])))

static lv_obj_t *s_box;                    /* compact red box, top of screen */
static lv_obj_t *s_line[WARN_MAX_SHOWN];
static uint8_t s_dbg_warn_shown;

void ui_warning_create(lv_obj_t *parent)
{
    /* Red box pinned to the top — clears the centered menu popup (which spans y≈130–350) */
    s_box = lv_obj_create(parent);
    lv_obj_set_size(s_box, 440, 120);
    lv_obj_align(s_box, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(s_box, COLOR_RED_HOT, 0);
    lv_obj_set_style_bg_opa(s_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_box, lv_color_black(), 0);
    lv_obj_set_style_border_width(s_box, 3, 0);
    lv_obj_set_style_radius(s_box, 10, 0);
    lv_obj_clear_flag(s_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_box, LV_OBJ_FLAG_HIDDEN);

    /* Title — black */
    lv_obj_t *title = lv_label_create(s_box);
    lv_label_set_text(title, "ECU WARNING");
    lv_obj_set_style_text_color(title, lv_color_black(), 0);
    lv_obj_set_style_text_font(title, &racehead_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Up to 3 warning lines — large black text */
    for (int i = 0; i < WARN_MAX_SHOWN; i++) {
        s_line[i] = lv_label_create(s_box);
        lv_label_set_text(s_line[i], "");
        lv_obj_set_style_text_color(s_line[i], lv_color_black(), 0);
        lv_obj_set_style_text_font(s_line[i], &racehead_17, 0);
        lv_obj_align(s_line[i], LV_ALIGN_TOP_MID, 0, 30 + i * 26);
    }
}

void ui_warning_update(const dash_data_t *d)
{
    if (d->warn == 0) {
        lv_obj_add_flag(s_box, LV_OBJ_FLAG_HIDDEN);
        s_dbg_warn_shown = 0;
        return;
    }
    // #region agent log
    if (!s_dbg_warn_shown) {
        UI_DBG_LOG("D", "ui_warning.c:update", "warn_show", "\"warn\":%d", (int)d->warn);
        s_dbg_warn_shown = 1;
    }
    // #endregion

    int shown = 0;
    for (int i = 0; i < WARN_DEFS_N && shown < WARN_MAX_SHOWN; i++) {
        if (d->warn & WARN_DEFS[i].bit) {
            lv_label_set_text(s_line[shown], WARN_DEFS[i].name);
            lv_obj_clear_flag(s_line[shown], LV_OBJ_FLAG_HIDDEN);
            shown++;
        }
    }
    for (int i = shown; i < WARN_MAX_SHOWN; i++)
        lv_obj_add_flag(s_line[i], LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(s_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_box);
}
