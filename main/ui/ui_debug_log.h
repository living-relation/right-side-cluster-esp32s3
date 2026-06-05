#pragma once
#include "esp_log.h"

/* Session d58ccd — structured serial logs for debug mode (parse from COM6 monitor). */
#define UI_DBG_TAG "d58ccd"

#define UI_DBG_LOG(hyp, loc, msg, fmt, ...) \
    ESP_LOGI(UI_DBG_TAG, "{\"sessionId\":\"d58ccd\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"%s\",\"data\":{" fmt "}}", \
             (hyp), (loc), (msg), ##__VA_ARGS__)
