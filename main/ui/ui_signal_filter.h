#pragma once

float ui_filter_boost(float raw);
float ui_filter_ign(float raw);
float ui_filter_lambda(float raw);
/** Last value returned by ui_filter_boost (same frame, for alarms / bg flash). */
float ui_filter_boost_last(void);
void ui_filter_reset(void);
