/**
 * alarm_task.c — 20 ms threshold scanner, right cluster.
 * Evaluates channels rendered on the right display only.
 */

#include "dash_data.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

extern portMUX_TYPE g_dash_mux;

void alarm_task(void *arg)
{
    TickType_t       wake   = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(20);

    for (;;) {
        dash_data_t s;
        portENTER_CRITICAL(&g_dash_mux);
        s = *(const dash_data_t *)&g_dash;
        portEXIT_CRITICAL(&g_dash_mux);

        uint16_t flags = s.flags;

        /* Skip alarm evaluation until first UART frame received (avoids false
         * boot alarms when g_dash is zero-initialised). */
        if (s.last_update_ms == 0) goto skip_alarms;

        /* Lambda alarm */
        if (s.lambda < DASH_LAMBDA_RICH_ALARM || s.lambda >= DASH_LAMBDA_LEAN_ALARM)
            flags |= DASH_FLAG_LAMBDA_BAD;
        else
            flags &= ~DASH_FLAG_LAMBDA_BAD;

        /* Overboost */
        if (s.boost >= DASH_BOOST_OVERBOOST)
            flags |= DASH_FLAG_OVERBOOST;
        else
            flags &= ~DASH_FLAG_OVERBOOST;

        /* Coolant hot */
        if (s.coolant_temp >= DASH_COOLANT_MAX)
            flags |= DASH_FLAG_COOLANT_HOT;
        else
            flags &= ~DASH_FLAG_COOLANT_HOT;

        /* Engine-protect flags — re-evaluated from forwarded ECU fields */
        if (s.knock_level > DASH_KNOCK_THRESHOLD)
            flags |= DASH_FLAG_KNOCK;
        else
            flags &= ~DASH_FLAG_KNOCK;
        if (s.boost_cut)
            flags |= DASH_FLAG_BOOST_CUT;
        else
            flags &= ~DASH_FLAG_BOOST_CUT;
        /* Fuel/ign cut: only alarm when protecting the engine, not at rev limit */
        if (s.fuel_cut_level > 0 && !s.rev_limit)
            flags |= DASH_FLAG_FUEL_CUT;
        else
            flags &= ~DASH_FLAG_FUEL_CUT;
        if (s.ign_cut_level > 0 && !s.rev_limit)
            flags |= DASH_FLAG_IGN_CUT;
        else
            flags &= ~DASH_FLAG_IGN_CUT;
        if (s.traction_cut)
            flags |= DASH_FLAG_TRACTION_CUT;
        else
            flags &= ~DASH_FLAG_TRACTION_CUT;
        if (s.rev_limit)
            flags |= DASH_FLAG_REV_LIMIT;
        else
            flags &= ~DASH_FLAG_REV_LIMIT;
        if (s.launch_ctrl)
            flags |= DASH_FLAG_LAUNCH;
        else
            flags &= ~DASH_FLAG_LAUNCH;

        portENTER_CRITICAL(&g_dash_mux);
        g_dash.flags = flags;
        portEXIT_CRITICAL(&g_dash_mux);

skip_alarms:
        vTaskDelayUntil(&wake, period);
    }
}
