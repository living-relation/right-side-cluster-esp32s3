#include "ui_numeric.h"
#include "lvgl.h"

static int frac1(float v)
{
    float w = (float)(int)v;
    int f = (int)((v - w) * 10.0f + 0.5f);
    if (f >= 10) {
        return 0;
    }
    return f;
}

static int frac3(float v)
{
    float w = (float)(int)v;
    int f = (int)((v - w) * 1000.0f + 0.5f);
    if (f >= 1000) {
        return 0;
    }
    return f;
}

void ui_fmt_1dp(char *buf, size_t buf_len, float v)
{
    if (buf_len == 0) {
        return;
    }
    int neg = (v < 0.0f) ? 1 : 0;
    if (neg) {
        v = -v;
    }
    int w = (int)v;
    int f = frac1(v);
    if (f >= 10) {
        w++;
        f -= 10;
    }
    if (neg) {
        lv_snprintf(buf, buf_len, "-%d.%d", w, f);
    } else {
        lv_snprintf(buf, buf_len, "%d.%d", w, f);
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
    int w = (int)v;
    int f = frac3(v);
    lv_snprintf(buf, buf_len, "%d.%03d", w, f);
}
