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

        portENTER_CRITICAL(&g_dash_mux);
        g_dash.flags = flags;
        portEXIT_CRITICAL(&g_dash_mux);

        vTaskDelayUntil(&wake, period);
    }
}
