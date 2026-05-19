/**
 * run2_config.h — Run_2 right firmware wiring and protocol constants
 *
 * Execution context:
 * - Repo: living-relation/right-side-cluster-esp32s3
 * - Branch: feature/Run_2_firmware_foundation
 * - Target: Waveshare ESP32-S3 2.8C round display, 480 x 480
 *
 * Right display is a dumb UART-only slave. It owns no CAN, controls, menu
 * state, analog inputs, GPS, lap timing, or independent display mode.
 * AFR is banned; lambda only.
 */

#ifndef RUN2_CONFIG_H
#define RUN2_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RUN2_PROJECT_TAG                    "Run_2"
#define RUN2_SIDE_DISPLAY_WIDTH_PX          480u
#define RUN2_SIDE_DISPLAY_HEIGHT_PX         480u

#define RUN2_UART_BAUD                      921600u
#define RUN2_UART_FRAME_HZ                  50u
#define RUN2_UART_PERIOD_MS                 20u
#define RUN2_UART_FRAME_TYPE_LEFT           0x01u
#define RUN2_UART_FRAME_TYPE_RIGHT          0x02u

/* Verified Waveshare 2.8C side-board UART header mapping. */
#define RUN2_SIDE_UART_RX_GPIO              44
#define RUN2_SIDE_UART_TX_GPIO_UNUSED       43

#define RUN2_SIDE_IS_DUMB_UART_SLAVE        1
#define RUN2_SIDE_OWNS_CAN                  0
#define RUN2_SIDE_OWNS_CONTROLS             0
#define RUN2_SIDE_OWNS_MENU_STATE           0
#define RUN2_SIDE_OWNS_ANALOG_INPUTS        0
#define RUN2_SIDE_OWNS_GPS                  0
#define RUN2_SIDE_OWNS_LAP_TIMER            0
#define RUN2_AFR_IS_BANNED                  1

#ifdef __cplusplus
}
#endif

#endif /* RUN2_CONFIG_H */
