#pragma once
#include "lvgl.h"
#include "dash_data.h"
void ui_warning_overlay_create(lv_obj_t *parent);
void ui_warning_overlay_update(const dash_data_t *d);
