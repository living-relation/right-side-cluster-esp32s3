/**
 * ui_menu_popup.h — encoder selection popup overlay, right cluster.
 *
 * Appears when g_dash.menu_id != DASH_MENU_NONE (forwarded from center
 * via UART frame reserved bytes). The popup scrolls to show menu options
 * with a centered highlight bar indicating the current selection.
 */
#ifndef UI_MENU_POPUP_H
#define UI_MENU_POPUP_H

#include "lvgl.h"
#include "dash_data.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_menu_popup_create(lv_obj_t *parent);
void ui_menu_popup_update(const dash_data_t *d);
void ui_menu_popup_reset_cache(void);

#ifdef __cplusplus
}
#endif
#endif
