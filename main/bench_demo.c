/**
 * bench_demo.c — right-cluster standalone demo (CONFIG_TC_BENCH_MODE).
 *
 * Two drive cycles, then warning + encoder menus:
 *   3 s highway cruise → WOT 3rd → shift → WOT 4th → 5th cruise 3 s →
 *   decel (blow-off ~300 ms) → repeat. IGN/ECT tuned for warm 3SGTE/5SGTE-like
 *   Link CAN values; display smoothing is ui_signal_filter (not here).
 */
#include "bench_demo.h"
#include "dash_data.h"
#include "menu_strings.h"
#include "ui/ui_menu_popup.h"
#include "ui/ui_signal_filter.h"
#include "sdkconfig.h"

#if CONFIG_TC_BENCH_MODE

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ui/ui_debug_log.h"
#include <math.h>

static const char *TAG = "bench_demo";

extern portMUX_TYPE g_dash_mux;

#define BENCH_TICK_MS           50
#define MAIN_CYCLES_TARGET      2
#define WARN_HOLD_MS            2000
#define MENU_STEP_MS            450

typedef enum {
    PHASE_MAIN = 0,
    PHASE_WARN,
    PHASE_MENU_TC,
    PHASE_MENU_BOOST,
} bench_phase_t;

typedef enum {
    SEG_CRUISE_HWY = 0,
    SEG_PULL_3RD,
    SEG_PULL_4TH,
    SEG_CRUISE_5TH,
    SEG_DECEL,
    SEG_COUNT,
} drive_seg_t;

static const uint32_t SEG_MS[SEG_COUNT] = {
    3000,  /* highway cruise before the pull */
    3500,  /* WOT 3rd → redline */
    3000,  /* upshift, WOT 4th → redline */
    3000,  /* upshift, 5th highway cruise */
    5000,  /* slow decel */
};

static TaskHandle_t s_task;
static bench_phase_t s_phase = PHASE_MAIN;
static drive_seg_t s_seg = SEG_CRUISE_HWY;
static uint8_t s_main_cycles;
static uint8_t s_menu_cursor;
static uint32_t s_phase_ms;
static uint8_t s_menu_id_active;
static uint8_t s_warn_preset_idx;

static const uint8_t WARN_PRESETS[] = {
    (uint8_t)(DASH_WARN_KNOCK | DASH_WARN_IGN_CUT | DASH_WARN_FUEL_CUT),
    (uint8_t)(DASH_WARN_BOOST_CUT | DASH_WARN_SENSOR),
    (uint8_t)(DASH_WARN_THROTTLE | DASH_WARN_KNOCK),
    (uint8_t)(DASH_WARN_IGN_CUT | DASH_WARN_BOOST_CUT),
    (uint8_t)(DASH_WARN_FUEL_CUT | DASH_WARN_SENSOR | DASH_WARN_THROTTLE),
};
#define WARN_PRESETS_N ((int)(sizeof(WARN_PRESETS) / sizeof(WARN_PRESETS[0])))

static void phase_enter(bench_phase_t ph);

static float clamp01(float t)
{
    if (t < 0.0f) {
        return 0.0f;
    }
    if (t > 1.0f) {
        return 1.0f;
    }
    return t;
}

static float smoothstep(float t)
{
    t = clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

static float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

static void apply_snapshot(void (*fill)(dash_data_t *d))
{
    portENTER_CRITICAL(&g_dash_mux);
    fill((dash_data_t *)&g_dash);
    g_dash.last_update_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    portEXIT_CRITICAL(&g_dash_mux);
}

static void fill_cruise_hwy(dash_data_t *d, float t)
{
    (void)t;
    /* Warm engine, cool day: thermostat-regulated ECT, moderate in-boost cruise timing. */
    d->boost        = 11.0f;
    d->coolant_temp = 194.0f;
    d->iat          = 88.0f;
    d->ign_adv      = 30.0f;
    d->lambda       = 0.96f;
}

static void fill_pull_3rd(dash_data_t *d, float t)
{
    t = clamp01(t);
    float load = smoothstep(t);

    if (t < 0.12f) {
        d->boost = lerp(11.0f, 4.0f, t / 0.12f);
    } else if (t < 0.55f) {
        float u = smoothstep((t - 0.12f) / 0.43f);
        d->boost = lerp(4.0f, 26.0f, u);
    } else {
        float u = (t - 0.55f) / 0.45f;
        d->boost = lerp(26.0f, 29.0f, smoothstep(u));
    }

    d->coolant_temp = lerp(194.0f, 200.0f, load);
    d->iat          = lerp(88.0f, 132.0f, load);
    /* Turbo: more boost → less advance (Link-style load axis). */
    d->ign_adv      = lerp(30.0f, 19.0f, powf(load, 0.85f));
    if (t < 0.20f) {
        d->lambda = lerp(0.96f, 0.84f, t / 0.20f);
    } else {
        d->lambda = lerp(0.84f, 0.93f, smoothstep((t - 0.20f) / 0.80f));
    }
}

static void fill_pull_4th(dash_data_t *d, float t)
{
    t = clamp01(t);
    float load = smoothstep(t);

    if (t < 0.10f) {
        d->boost = lerp(28.0f, 6.0f, t / 0.10f);
    } else if (t < 0.50f) {
        float u = smoothstep((t - 0.10f) / 0.40f);
        d->boost = lerp(6.0f, 24.0f, u);
    } else {
        float u = (t - 0.50f) / 0.50f;
        d->boost = lerp(24.0f, 27.0f, smoothstep(u));
    }

    d->coolant_temp = lerp(200.0f, 202.0f, load);
    d->iat          = lerp(132.0f, 145.0f, load);
    d->ign_adv      = lerp(20.0f, 22.0f, load);
    d->lambda       = lerp(0.93f, 0.95f, load);
}

static void fill_cruise_5th(dash_data_t *d, float t)
{
    (void)t;
    d->boost        = 13.5f;
    d->coolant_temp = 200.0f;
    d->iat          = 118.0f;
    d->ign_adv      = 27.0f;
    d->lambda       = 0.97f;
}

static void fill_decel(dash_data_t *d, float t)
{
    float tc = clamp01(t);

    /* Throttle lift: blow-off to vacuum in ~300 ms, then hold. */
    if (tc < 0.06f) {
        d->boost = lerp(13.5f, 0.0f, smoothstep(tc / 0.06f));
    } else {
        d->boost = 0.0f;
    }

    t = smoothstep(tc);
    d->coolant_temp = lerp(200.0f, 196.0f, t);
    d->iat          = lerp(118.0f, 82.0f, t);
    d->ign_adv      = lerp(27.0f, 16.0f, t);
    d->lambda       = lerp(0.97f, 1.02f, t);
}

static void fill_segment(dash_data_t *d)
{
    uint32_t span = SEG_MS[s_seg];
    float t = (span > 0) ? ((float)s_phase_ms / (float)span) : 0.0f;

    switch (s_seg) {
    case SEG_CRUISE_HWY:
        fill_cruise_hwy(d, t);
        break;
    case SEG_PULL_3RD:
        fill_pull_3rd(d, t);
        break;
    case SEG_PULL_4TH:
        fill_pull_4th(d, t);
        break;
    case SEG_CRUISE_5TH:
        fill_cruise_5th(d, t);
        break;
    case SEG_DECEL:
    default:
        fill_decel(d, t);
        break;
    }
}

static void fill_main(dash_data_t *d)
{
    d->menu_id = DASH_MENU_NONE;
    d->menu_cursor = 0;
    d->warn = 0;
    fill_segment(d);
}

static void fill_warn(dash_data_t *d)
{
    d->menu_id = DASH_MENU_NONE;
    d->menu_cursor = 0;
    fill_segment(d);
    d->warn = WARN_PRESETS[s_warn_preset_idx % WARN_PRESETS_N];
    s_warn_preset_idx++;
}

static void fill_menu(dash_data_t *d)
{
    fill_segment(d);
    d->warn = 0;
    d->menu_id = s_menu_id_active;
    d->menu_cursor = s_menu_cursor;
}

static int menu_count(uint8_t menu_id)
{
    return (menu_id == DASH_MENU_BOOST) ? BOOST_MAP_COUNT : TC_SLIP_COUNT;
}

static void segment_advance(void)
{
    if (s_seg + 1 < SEG_COUNT) {
        s_seg = (drive_seg_t)(s_seg + 1);
        s_phase_ms = 0;
        // #region agent log
        UI_DBG_LOG("H", "bench_demo.c:segment_advance", "seg",
                   "\"seg\":%d,\"cycle\":%d", (int)s_seg, (int)s_main_cycles);
        // #endregion
        return;
    }

    s_main_cycles++;
    if (s_main_cycles >= MAIN_CYCLES_TARGET) {
        phase_enter(PHASE_WARN);
        return;
    }

    s_seg = SEG_CRUISE_HWY;
    s_phase_ms = 0;
    ui_filter_reset();
    // #region agent log
    UI_DBG_LOG("H", "bench_demo.c:segment_advance", "lap",
               "\"cycle\":%d", (int)s_main_cycles);
    // #endregion
}

static void phase_enter(bench_phase_t ph)
{
    s_phase = ph;
    s_phase_ms = 0;
    if (ph == PHASE_MAIN) {
        s_seg = SEG_CRUISE_HWY;
        s_main_cycles = 0;
        ui_filter_reset();
        apply_snapshot(fill_main);
    } else if (ph == PHASE_WARN) {
        apply_snapshot(fill_warn);
    } else if (ph == PHASE_MENU_TC) {
        s_menu_id_active = DASH_MENU_TC;
        s_menu_cursor = 0;
        apply_snapshot(fill_menu);
    } else if (ph == PHASE_MENU_BOOST) {
        s_menu_id_active = DASH_MENU_BOOST;
        s_menu_cursor = 0;
        apply_snapshot(fill_menu);
    }
    ESP_LOGI(TAG, "phase %d seg %d", (int)ph, (int)s_seg);
    // #region agent log
    UI_DBG_LOG("C", "bench_demo.c:phase_enter", "phase",
               "\"ph\":%d,\"seg\":%d,\"boost\":%d", (int)ph, (int)s_seg,
               (int)g_dash.boost);
    // #endregion
    if (ph == PHASE_MENU_TC || ph == PHASE_MENU_BOOST) {
        ui_menu_popup_reset_cache();
    }
}

static bool menu_advance(bench_phase_t next_phase)
{
    int n = menu_count(s_menu_id_active);
    if (s_phase_ms < MENU_STEP_MS) {
        return false;
    }
    s_phase_ms = 0;
    if (s_menu_cursor + 1 < (uint8_t)n) {
        s_menu_cursor++;
        apply_snapshot(fill_menu);
        return false;
    }
    phase_enter(next_phase);
    return true;
}

static void bench_step(void)
{
    s_phase_ms += BENCH_TICK_MS;

    switch (s_phase) {
    case PHASE_MAIN:
        if (s_phase_ms >= SEG_MS[s_seg]) {
            segment_advance();
            if (s_phase != PHASE_MAIN) {
                return;
            }
        }
        apply_snapshot(fill_main);
        break;

    case PHASE_WARN:
        if (s_phase_ms >= WARN_HOLD_MS) {
            phase_enter(PHASE_MENU_TC);
        }
        break;

    case PHASE_MENU_TC:
        (void)menu_advance(PHASE_MENU_BOOST);
        break;

    case PHASE_MENU_BOOST:
        (void)menu_advance(PHASE_MAIN);
        break;

    default:
        break;
    }
}

static void bench_task(void *arg)
{
    ESP_LOGI(TAG, "bench demo task running");
    phase_enter(PHASE_MAIN);
    for (;;) {
        bench_step();
        vTaskDelay(pdMS_TO_TICKS(BENCH_TICK_MS));
    }
}

void bench_demo_start(void)
{
    if (s_task) {
        return;
    }
    xTaskCreatePinnedToCore(bench_task, "bench", 8192, NULL, 7, &s_task, 0);
}

#endif /* CONFIG_TC_BENCH_MODE */
