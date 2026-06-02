/**
 * ui_warning.h — full-screen ECU engine-protection warning overlay, right cluster.
 */
#ifndef UI_WARNING_H
#define UI_WARNING_H

#include "lvgl.h"
#include "dash_data.h"

void ui_warning_create(lv_obj_t *parent);
/* Shows the yellow/red/black overlay listing up to 3 highest-priority active
 * warnings when d->warn != 0; hides it otherwise. */
void ui_warning_update(const dash_data_t *d);

#endif /* UI_WARNING_H */
