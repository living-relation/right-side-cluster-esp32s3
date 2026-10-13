#include "ui_numeric.h"
#include "lvgl.h"

/* Round once to an integer count of the shown unit, then split it, so a
 * carry (0.9996 -> 1.000, 12.97 -> 13.0) reaches the whole part. */

void ui_fmt_1dp(char *buf, size_t buf_len, float v)
{
    if (buf_len == 0) {
        return;
    }
    int neg = (v < 0.0f) ? 1 : 0;
    if (neg) {
        v = -v;
    }
    int t = (int)(v * 10.0f + 0.5f);
    if (neg && t > 0) {
        lv_snprintf(buf, buf_len, "-%d.%d", t / 10, t % 10);
    } else {
        lv_snprintf(buf, buf_len, "%d.%d", t / 10, t % 10);
    }
}

void ui_fmt_3dp(char *buf, size_t buf_len, float v)
{
    if (buf_len == 0) {
        return;
    }
    if (v < 0.0f) {
        v = 0.0f;
    }
    int t = (int)(v * 1000.0f + 0.5f);
    lv_snprintf(buf, buf_len, "%d.%03d", t / 1000, t % 1000);
}
