/**
 * right-dash_data.h — TrackCluster Run_2 right-side dash data model
 *
 * Execution context:
 * - Repo: living-relation/right-side-cluster-esp32s3
 * - Branch: feature/Run_2_firmware_foundation
 * - Role: CONSUMER. Right display is a dumb UART-only display slave.
 *
 * AFR is banned. Lambda only.
 */

#ifndef RIGHT_DASH_DATA_H
#define RIGHT_DASH_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "run2_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DASH_FLAG_OIL_PRESS_LOW  (1u << 0)
#define DASH_FLAG_FUEL_LOW       (1u << 1)
#define DASH_FLAG_SHIFT          (1u << 2)
#define DASH_FLAG_REV_LIMIT      (1u << 3)
#define DASH_FLAG_COOLANT_HOT    (1u << 4)
#define DASH_FLAG_OIL_TEMP_HOT   (1u << 5)
#define DASH_FLAG_LAMBDA_BAD     (1u << 6)
#define DASH_FLAG_OVERBOOST      (1u << 7)

typedef enum {
    DASH_ODO     = 0,
    DASH_TRIP_A  = 1,
    DASH_TRIP_B  = 2,
} dash_odo_mode_t;

typedef enum {
    DASH_PROTECT_LEVEL_NONE     = 0,
    DASH_PROTECT_LEVEL_WARNING  = 1,
    DASH_PROTECT_LEVEL_PROTECT  = 2,
    DASH_PROTECT_LEVEL_CRITICAL = 3,
} dash_protect_level_t;

typedef struct {
    /* Engine */
    float    rpm;
    float    lambda;       /* right display lambda value; AFR is banned */
    float    boost;        /* direct ECU boost PSI through center model */
    float    ign_adv;

    /* Temperatures, degrees F */
    float    oil_temp;
    float    coolant_temp;
    float    iat;

    /* Pressures, PSI */
    float    oil_press;
    float    fuel_press;

    /* Driveline */
    float    mph;
    int8_t   gear;

    /* Fuel */
    float    fuel_level;

    /* Trip / odo */
    float    odo;
    float    trip_a;
    float    trip_b;
    uint8_t  odo_mode;

    /* Center-owned UI/control state forwarded to dumb side displays */
    uint8_t  display_mode;
    uint8_t  headlight_dim;
    uint8_t  boost_menu_index;
    uint8_t  tc_menu_index;

    /* Local display flags and ECU/global warning state */
    uint16_t flags;
    uint32_t ecu_protect_flags;
    uint32_t ecu_warning_flags;
    uint8_t  ecu_limp_mode;
    uint8_t  ecu_protect_level;
    uint8_t  ecu_last_protect_reason;
    uint32_t ecu_status_counter;

    uint32_t last_update_ms;
} dash_data_t;

extern volatile dash_data_t dash;

#define UART_BRIDGE_BAUD         RUN2_UART_BAUD
#define UART_BRIDGE_FRAME_LEN    32u
#define UART_BRIDGE_SOF          0xA5u
#define UART_BRIDGE_EOF          0x5Au
#define UART_BRIDGE_TYPE_LEFT    RUN2_UART_FRAME_TYPE_LEFT
#define UART_BRIDGE_TYPE_RIGHT   RUN2_UART_FRAME_TYPE_RIGHT
#define UART_BRIDGE_PERIOD_MS    RUN2_UART_PERIOD_MS

uint8_t dash_crc8(const uint8_t *data, size_t n);
void dash_encode_left (uint8_t out[UART_BRIDGE_FRAME_LEN], const dash_data_t *d, uint16_t seq);
void dash_encode_right(uint8_t out[UART_BRIDGE_FRAME_LEN], const dash_data_t *d, uint16_t seq);
bool dash_decode_left (const uint8_t in[UART_BRIDGE_FRAME_LEN], dash_data_t *d, uint16_t *seq_out);
bool dash_decode_right(const uint8_t in[UART_BRIDGE_FRAME_LEN], dash_data_t *d, uint16_t *seq_out);

#define DASH_RPM_SHIFT_SLOW          5500.0f
#define DASH_RPM_SHIFT_FAST          6500.0f
#define DASH_RPM_LEDS_ALL_RED_ON     7000.0f
#define DASH_RPM_LEDS_ALL_RED_OFF    6500.0f
#define DASH_RPM_REDLINE             6750.0f
#define DASH_RPM_REV_LIMIT           7500.0f

#define DASH_OIL_TEMP_REDLINE        215.0f
#define DASH_OIL_TEMP_MAX            250.0f
#define DASH_OIL_PRESS_MIN           25.0f
#define DASH_OIL_PRESS_MIN_RPM       2000.0f
#define DASH_OIL_PRESS_REDLINE       125.0f

#define DASH_FUEL_PRESS_DISPLAY_MIN  20.0f
#define DASH_FUEL_PRESS_DISPLAY_MAX  160.0f
#define DASH_FUEL_PRESS_LOW          30.0f
#define DASH_FUEL_PRESS_REDLINE      125.0f

#define DASH_ECT_GREEN_MAX           200.0f
#define DASH_ECT_YELLOW_MAX          215.0f
#define DASH_ECT_ORANGE_MAX          230.0f
#define DASH_COOLANT_MAX             230.0f

#define DASH_IAT_DISPLAY_MIN         30.0f
#define DASH_IAT_DISPLAY_MAX         200.0f
#define DASH_IAT_GREEN_MAX           100.0f
#define DASH_IAT_YELLOW_MAX          130.0f
#define DASH_IAT_ORANGE_MAX          150.0f

#define DASH_BOOST_CYAN_MAX          14.0f
#define DASH_BOOST_ORANGE_MAX        20.0f
#define DASH_BOOST_OVERBOOST         22.0f

#define DASH_LAMBDA_RICH_ALARM       0.70f
#define DASH_LAMBDA_RICH_WARN        0.75f
#define DASH_LAMBDA_LEAN_WARN        1.02f
#define DASH_LAMBDA_LEAN_ALARM       1.05f

#define DASH_IGN_GREEN_MIN           12.0f
#define DASH_IGN_GREEN_MAX           32.0f

#define DASH_FUEL_LOW                15.0f
#define DASH_FUEL_CAUTION            30.0f

#define DASH_STALE_MS                500u
#define DASH_STALE_OFF_MS            2000u

#ifdef __cplusplus
}
#endif

#endif /* RIGHT_DASH_DATA_H */
