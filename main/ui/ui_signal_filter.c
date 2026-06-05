/**
 * Display-only smoothing for right-cluster gauges (raw g_dash unchanged).
 *
 * Real-world strategy (bench + live UART share this path):
 * - Boost: asymmetric — fast attack for spool/peaks, fast fall for throttle lift
 *   (large negative steps snap or high blend), heavy damping only on tiny ripples
 *   at steady boost so the bar does not flicker on sensor/telemetry noise.
 * - IGN: moderate symmetric EMA with slightly faster fall than rise so lift/retard
 *   reads promptly; small snap on large steps (ECU map cell changes).
 * - Lambda: heavy EMA — wideband is noisy; prioritize stable readout over latency.
 *
 * Thresholds are tuned for ~50 ms UI paint; if paint rate changes, revisit blends.
 */
#include "ui_signal_filter.h"
#include "ui_debug_log.h"
#include <math.h>

#define UNINIT (-999.0f)

/* Boost fall tiers (PSI per 50 ms tick, typical). */
#define BOOST_FALL_SNAP_PSI    8.0f
#define BOOST_FALL_FAST_PSI    1.5f
#define BOOST_FALL_FAST_BLEND  0.62f
#define BOOST_FALL_SNAP_BLEND  1.0f

static float s_boost      = UNINIT;
static float s_boost_last = UNINIT;
static float s_ign        = UNINIT;
static float s_lambda     = UNINIT;

static float ema_asym(float raw, float *state, float rise_blend, float fall_blend, float snap)
{
    if (*state < -900.0f) {
        *state = raw;
        return raw;
    }
    float delta = raw - *state;
    if (fabsf(delta) >= snap) {
        *state = raw;
    } else if (delta > 0.0f) {
        *state += delta * rise_blend;
    } else {
        *state += delta * fall_blend;
    }
    return *state;
}

void ui_filter_reset(void)
{
    s_boost      = UNINIT;
    s_boost_last = UNINIT;
    s_ign        = UNINIT;
    s_lambda     = UNINIT;
}

float ui_filter_boost_last(void)
{
    return (s_boost_last < -900.0f) ? 0.0f : s_boost_last;
}

float ui_filter_boost(float raw)
{
    if (s_boost < -900.0f) {
        s_boost = raw;
        s_boost_last = raw;
        return raw;
    }

    float delta = raw - s_boost;
    float prev = s_boost;

    if (delta >= 4.0f) {
        s_boost += delta * 0.82f;
    } else if (delta >= 0.2f) {
        s_boost += delta * 0.58f;
    } else if (delta > 0.02f) {
        s_boost += delta * 0.32f;
    } else if (delta <= -BOOST_FALL_SNAP_PSI) {
        s_boost = raw;
        // #region agent log
        if (prev > 5.0f && raw < 2.0f) {
            UI_DBG_LOG("I", "ui_signal_filter.c:ui_filter_boost", "blowoff_snap",
                       "\"prev\":%d,\"raw\":%d,\"delta\":%d",
                       (int)(prev * 10.0f), (int)(raw * 10.0f), (int)(delta * 10.0f));
        }
        // #endregion
    } else if (delta <= -BOOST_FALL_FAST_PSI) {
        s_boost += delta * BOOST_FALL_FAST_BLEND;
    } else if (delta < -0.02f) {
        s_boost += delta * 0.10f;
    }

    s_boost_last = s_boost;
    return s_boost;
}

float ui_filter_ign(float raw)
{
    return ema_asym(raw, &s_ign, 0.30f, 0.42f, 3.0f);
}

float ui_filter_lambda(float raw)
{
    return ema_asym(raw, &s_lambda, 0.14f, 0.14f, 0.025f);
}
