/**
 * ui_menu_popup.c — encoder selection popup overlay, right cluster.
 *
 * Triggered by g_dash.menu_id != DASH_MENU_NONE, which is set by the center
 * cluster's inputs.c and forwarded in the RIGHT UART frame (reserved bytes
 * 14-15). The center sends menu_id (which menu) and menu_cursor (highlighted
 * item index); the right display owns the option strings.
 *
 * Layout (480×480):
 *   Semi-transparent dark overlay behind a centred white-bordered box.
 *   Box shows a scrollable 3-item window:
 *     item above cursor   — dim white, small font
 *     cursor item         — bright white, larger font, highlight bg
 *     item below cursor   — dim white, small font
 *   Title at top of box (e.g. "BOOST MAP" or "TC SLIP ANGLE").
 *   Footer: "PRESS TO SELECT" in small dim text.
 *
 * Option strings must match inputs.c BOOST_MAPS[] / TC_SLIPS[] on center.
 */

#include "ui_menu_popup.h"
#include "ui_debug_log.h"
#include "right-colors.h"
#include "sdkconfig.h"
#include "../menu_strings.h"   /* shared with center inputs.c — keep byte-identical */

LV_FONT_DECLARE(racehead_12);
LV_FONT_DECLARE(racehead_14);
/* Menu option strings include digits and ° — racehead_14/17 lack 0–9; use Montserrat for items. */
#if CONFIG_LV_FONT_MONTSERRAT_14
LV_FONT_DECLARE(lv_font_montserrat_14);
#define MENU_ITEM_FONT (&lv_font_montserrat_14)
#else
LV_FONT_DECLARE(racehead_14);
#define MENU_ITEM_FONT (&racehead_14)
#endif

/* ── Widget handles ──────────────────────────────────────────────────── */
static lv_obj_t *s_overlay   = NULL;   /* full-screen dim layer */
static lv_obj_t *s_box       = NULL;   /* popup container */
static lv_obj_t *s_title     = NULL;
static lv_obj_t *s_item_prev = NULL;   /* item above cursor */
static lv_obj_t *s_item_cur  = NULL;   /* highlighted item */
static lv_obj_t *s_item_next = NULL;   /* item below cursor */
static lv_obj_t *s_footer    = NULL;

static uint8_t s_last_menu_id     = 0xFF;
static uint8_t s_last_menu_cursor = 0xFF;
static uint8_t s_dbg_menu_shown;

/* ── Create ──────────────────────────────────────────────────────────── */
void ui_menu_popup_create(lv_obj_t *parent)
{
    /* Full-screen dim overlay */
    s_overlay = lv_obj_create(parent);
    lv_obj_set_size(s_overlay, 480, 480);
    lv_obj_center(s_overlay);
    lv_obj_set_style_bg_color(s_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_overlay, (lv_opa_t)(0.65f * 255), 0);
    lv_obj_set_style_border_width(s_overlay, 0, 0);
    lv_obj_set_style_radius(s_overlay, 0, 0);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_overlay);

    /* Popup box */
    s_box = lv_obj_create(s_overlay);
    lv_obj_set_size(s_box, 340, 220);
    lv_obj_center(s_box);
    lv_obj_set_style_bg_color(s_box, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_opa(s_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_box, COLOR_WHITE, 0);
    lv_obj_set_style_border_opa(s_box, LV_OPA_80, 0);
    lv_obj_set_style_border_width(s_box, 2, 0);
    lv_obj_set_style_radius(s_box, 8, 0);
    lv_obj_set_style_pad_all(s_box, 10, 0);
    lv_obj_clear_flag(s_box, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    s_title = lv_label_create(s_box);
    lv_label_set_text(s_title, "");
    lv_obj_set_style_text_color(s_title, COLOR_WHITE, 0);
    lv_obj_set_style_text_opa(s_title, LV_OPA_70, 0);
    lv_obj_set_style_text_font(s_title, &racehead_12, 0);
    lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 0);

    /* Item above cursor (dim) */
    s_item_prev = lv_label_create(s_box);
    lv_label_set_text(s_item_prev, "");
    lv_obj_set_style_text_color(s_item_prev, COLOR_WHITE, 0);
    lv_obj_set_style_text_opa(s_item_prev, LV_OPA_40, 0);
    lv_obj_set_style_text_font(s_item_prev, MENU_ITEM_FONT, 0);
    lv_obj_align(s_item_prev, LV_ALIGN_TOP_MID, 0, 28);

    /* Current (highlighted) item — bright, with accent background strip */
    lv_obj_t *highlight = lv_obj_create(s_box);
    lv_obj_set_size(highlight, 310, 40);
    lv_obj_align(highlight, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(highlight, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(highlight, (lv_opa_t)(0.12f * 255), 0);
    lv_obj_set_style_border_color(highlight, COLOR_WHITE, 0);
    lv_obj_set_style_border_opa(highlight, LV_OPA_30, 0);
    lv_obj_set_style_border_width(highlight, 1, 0);
    lv_obj_set_style_radius(highlight, 4, 0);
    lv_obj_clear_flag(highlight, LV_OBJ_FLAG_SCROLLABLE);

    s_item_cur = lv_label_create(highlight);
    lv_label_set_text(s_item_cur, "");
    lv_obj_set_style_text_color(s_item_cur, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(s_item_cur, MENU_ITEM_FONT, 0);
    lv_obj_center(s_item_cur);

    /* Item below cursor (dim) */
    s_item_next = lv_label_create(s_box);
    lv_label_set_text(s_item_next, "");
    lv_obj_set_style_text_color(s_item_next, COLOR_WHITE, 0);
    lv_obj_set_style_text_opa(s_item_next, LV_OPA_40, 0);
    lv_obj_set_style_text_font(s_item_next, MENU_ITEM_FONT, 0);
    lv_obj_align(s_item_next, LV_ALIGN_BOTTOM_MID, 0, -28);

    /* Footer */
    s_footer = lv_label_create(s_box);
    lv_label_set_text(s_footer, "PRESS TO SELECT");
    lv_obj_set_style_text_color(s_footer, COLOR_WHITE, 0);
    lv_obj_set_style_text_opa(s_footer, LV_OPA_30, 0);
    lv_obj_set_style_text_font(s_footer, &racehead_12, 0);
    lv_obj_align(s_footer, LV_ALIGN_BOTTOM_MID, 0, -2);
}

void ui_menu_popup_reset_cache(void)
{
    s_last_menu_id     = 0xFF;
    s_last_menu_cursor = 0xFF;
    s_dbg_menu_shown   = 0;
}

/* ── Update (called from ui_paint_tick) ─────────────────────────────── */
void ui_menu_popup_update(const dash_data_t *d)
{
    bool visible = (d->menu_id != DASH_MENU_NONE);

    /* Toggle overlay visibility */
    bool currently_hidden = lv_obj_has_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    if (visible && currently_hidden) {
        lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_overlay);
    }
    if (!visible && !currently_hidden) {
        lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    if (!visible) {
        s_last_menu_id = 0xFF;
        return;
    }
    lv_obj_move_foreground(s_overlay);
    // #region agent log
    if (!s_dbg_menu_shown || s_last_menu_id != d->menu_id) {
        UI_DBG_LOG("E", "ui_menu_popup.c:update", "menu_show",
                   "\"menu\":%d,\"cursor\":%d", (int)d->menu_id, (int)d->menu_cursor);
        s_dbg_menu_shown = 1;
    }
    // #endregion

    /* Only re-render if state changed */
    if (d->menu_id == s_last_menu_id && d->menu_cursor == s_last_menu_cursor) return;
    s_last_menu_id     = d->menu_id;
    s_last_menu_cursor = d->menu_cursor;

    const char * const *items;
    int count;
    const char *title;

    if (d->menu_id == DASH_MENU_BOOST) {
        items = BOOST_MAPS; count = BOOST_MAP_COUNT; title = "BOOST MAP";
    } else {
        items = TC_SLIPS;   count = TC_SLIP_COUNT;   title = "TC SLIP ANGLE";
    }

    lv_label_set_text(s_title, title);

    int cur = (int)d->menu_cursor;
    if (cur < 0) cur = 0;
    if (cur >= count) cur = count - 1;

    lv_label_set_text(s_item_cur,  items[cur]);
    lv_label_set_text(s_item_prev, cur > 0          ? items[cur - 1] : "");
    lv_label_set_text(s_item_next, cur < count - 1  ? items[cur + 1] : "");
    // #region agent log
    UI_DBG_LOG("F", "ui_menu_popup.c:update", "menu_text",
               "\"cur\":%d,\"txt\":\"%s\"", cur, items[cur]);
    // #endregion
}
