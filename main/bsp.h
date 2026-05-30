/**
 * bsp.h — board support for Waveshare ESP32-S3-Touch-LCD-2.8C (left cluster).
 *
 * Display: 480×480 round, ST7701S, RGB-565 16-bit parallel interface.
 * Touch:   GT911 capacitive, I²C (SCL=GPIO7, SDA=GPIO15, INT=GPIO16).
 * Panel init: SPI-like 3-wire over GPIO1 (SDA) / GPIO2 (SCK);
 *             CS and RST via TCA9554 I²C expander (addr 0x20).
 *
 * Backlight: GPIO6 (BL_PWM) — controlled by firmware LEDC (8-bit, 1 kHz).
 *   255 = 100 % day brightness, 90 ≈ 35 % night brightness.
 *   Call bsp_set_brightness() after each UART frame to track center's setting.
 */
#ifndef BSP_H
#define BSP_H

#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_LCD_H_RES    480
#define BSP_LCD_V_RES    480
#define BSP_BL_PWM_GPIO    6   /* backlight PWM — LEDC channel 0, 8-bit, 1 kHz */

esp_err_t  bsp_init(void);
lv_disp_t *bsp_display_start(void);
bool        bsp_lvgl_lock(uint32_t timeout_ms);
void        bsp_lvgl_unlock(void);
/** Set backlight brightness. duty = 0–255 (255 = full, 90 ≈ 35 % night). */
void        bsp_set_brightness(uint8_t duty);

#ifdef __cplusplus
}
#endif
#endif
