#pragma once
#include "freertos/FreeRTOS.h"

/* Cross-core spinlock protecting struct-coherent access to the global `dash`
 * instance on the dual-core ESP32-S3. Every task that reads or writes `dash`
 * fields must acquire g_dash_mux via portENTER_CRITICAL / portEXIT_CRITICAL
 * before touching the struct. */
extern portMUX_TYPE g_dash_mux;
