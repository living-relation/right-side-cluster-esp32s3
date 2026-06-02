#pragma once
#include "lvgl.h"
#include "dash_data.h"
void ui_bar_gauges_create(lv_obj_t *parent);
void ui_bar_gauges_update(const dash_data_t *d);
