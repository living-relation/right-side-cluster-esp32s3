#ifndef UI_NUMERIC_H
#define UI_NUMERIC_H

#include <stddef.h>

/* LVGL labels: aerospace fonts only include 0-9 and '.' — no %f (LV_USE_FLOAT off). */
void ui_fmt_1dp(char *buf, size_t buf_len, float v);
void ui_fmt_3dp(char *buf, size_t buf_len, float v);

#endif
