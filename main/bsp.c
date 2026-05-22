/**
 * bsp.c — Waveshare ESP32-S3-Touch-LCD-2.8C board support (left cluster).
 *
 * Schematic-verified pin assignments:
 *
 * ST7701S RGB parallel bus (16-bit 565, driven by esp_lcd_rgb_panel):
 *   R1=46  R2=3   R3=8   R4=18  R5=17
 *   G0=14  G1=13  G2=12  G3=11  G4=10  G5=9
 *   B1=5   B2=45  B3=48  B4=47  B5=21
 *   PCLK=41  DE=40  VSYNC=39  HSYNC=38
 *
 * Panel SPI init (3-wire):
 *   SDA=GPIO1  SCK=GPIO2
 *   CS  → TCA9554 P2  (I²C expander, addr 0x20)
 *   RST → TCA9554 P0
 *
 * I²C shared bus (SCL=GPIO7, SDA=GPIO15):
 *   TCA9554 @ 0x20 — LCD_RST(P0), TP_RST(P1), LCD_CS(P2), SD_CS(P4)
 *   GT911   @ 0x5D or 0x14 — capacitive touch
 *
 * Touch: GT911  INT=GPIO16  RST→TCA9554 P1
 *
 * Backlight: GPIO6 = BL_PWM — driven by external PWM controller.
 *            Firmware NEVER touches GPIO6.
 */

#include "bsp.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lvgl_port.h"

static const char *TAG = "bsp";

/* ── GPIO constants (schematic-verified) ─────────────────────────────── */
#define BSP_LCD_SDA_GPIO     1   /* panel SPI init data  */
#define BSP_LCD_SCK_GPIO     2   /* panel SPI init clock */
#define BSP_PCLK_GPIO       41
#define BSP_DE_GPIO         40
#define BSP_VSYNC_GPIO      39
#define BSP_HSYNC_GPIO      38
#define BSP_R1_GPIO         46
#define BSP_R2_GPIO          3
#define BSP_R3_GPIO          8
#define BSP_R4_GPIO         18
#define BSP_R5_GPIO         17
#define BSP_G0_GPIO         14
#define BSP_G1_GPIO         13
#define BSP_G2_GPIO         12
#define BSP_G3_GPIO         11
#define BSP_G4_GPIO         10
#define BSP_G5_GPIO          9
#define BSP_B1_GPIO          5
#define BSP_B2_GPIO         45
#define BSP_B3_GPIO         48
#define BSP_B4_GPIO         47
#define BSP_B5_GPIO         21

/* TCA9554 I²C expander (addr 0x20) port bits */
#define TCA9554_ADDR        0x20
#define TCA9554_P_LCD_RST   (1 << 0)
#define TCA9554_P_TP_RST    (1 << 1)
#define TCA9554_P_LCD_CS    (1 << 2)

/* TCA9554 register addresses */
#define TCA9554_REG_INPUT   0x00
#define TCA9554_REG_OUTPUT  0x01
#define TCA9554_REG_CONFIG  0x03

static i2c_master_bus_handle_t  s_i2c_bus    = NULL;
static i2c_master_dev_handle_t  s_tca9554    = NULL;
static esp_lcd_panel_handle_t   s_panel      = NULL;
static esp_lcd_panel_io_handle_t s_panel_io  = NULL;

/* ── TCA9554 helpers ──────────────────────────────────────────────────── */
static uint8_t s_tca_output = 0xFF;   /* start all high */

static esp_err_t tca9554_write_output(uint8_t val)
{
    uint8_t buf[2] = { TCA9554_REG_OUTPUT, val };
    s_tca_output = val;
    return i2c_master_transmit(s_tca9554, buf, sizeof(buf), 50);
}

static esp_err_t tca9554_set_pin(uint8_t pin_mask, bool high)
{
    uint8_t v = high ? (s_tca_output | pin_mask) : (s_tca_output & ~pin_mask);
    return tca9554_write_output(v);
}

/* ── I²C bus + expander init ─────────────────────────────────────────── */
static esp_err_t i2c_and_expander_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .i2c_port          = I2C_NUM_0,
        .scl_io_num        = CONFIG_TC_I2C_SCL_GPIO,
        .sda_io_num        = CONFIG_TC_I2C_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_i2c_bus), TAG, "i2c_bus");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = TCA9554_ADDR,
        .scl_speed_hz    = 400000,
    };
    ESP_RETURN_ON_ERROR(
        i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, &s_tca9554), TAG, "tca9554");

    /* All pins as outputs, start high */
    uint8_t cfg_buf[2] = { TCA9554_REG_CONFIG, 0x00 };
    ESP_RETURN_ON_ERROR(
        i2c_master_transmit(s_tca9554, cfg_buf, sizeof(cfg_buf), 50), TAG, "tca_cfg");
    return tca9554_write_output(0xFF);
}

/* ── ST7701S panel init (3-wire SPI via bit-bang + TCA9554 CS) ──────── */
static esp_err_t panel_init(void)
{
    /* Assert LCD_RST low, then high */
    ESP_RETURN_ON_ERROR(tca9554_set_pin(TCA9554_P_LCD_RST, false), TAG, "rst_lo");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(tca9554_set_pin(TCA9554_P_LCD_RST, true),  TAG, "rst_hi");
    vTaskDelay(pdMS_TO_TICKS(120));

    /* Panel SPI IO (uses GPIO bit-bang via esp_lcd_panel_io_spi) */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num  = -1,   /* CS driven via TCA9554 */
        .dc_gpio_num  = -1,   /* 3-wire SPI: no DC pin */
        .spi_clock_hz = 500000,
        .spi_mode     = 0,
        .pclk_hz      = 500000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits   = 8,
        .lcd_param_bits = 8,
        .flags.dc_low_on_data = 0,
        .flags.octal_mode     = 0,
        .flags.sio_mode       = 1,   /* 3-wire: cmd+data on SDA */
    };

    /* Assert CS before panel commands */
    ESP_RETURN_ON_ERROR(tca9554_set_pin(TCA9554_P_LCD_CS, false), TAG, "cs_lo");

    /* NOTE: For ST7701S the full init sequence (vendor commands) is handled
     * by the esp_lcd_st7701 managed component. The component calls the
     * vendor_config init_cmds[] table on esp_lcd_panel_init(). */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = -1,   /* RST driven via TCA9554 above */
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    /* RGB panel config (the actual pixel data path) */
    esp_lcd_rgb_panel_config_t rgb_cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz        = 16000000,
            .h_res          = BSP_LCD_H_RES,
            .v_res          = BSP_LCD_V_RES,
            .hsync_pulse_width = 10,
            .hsync_back_porch  = 10,
            .hsync_front_porch = 20,
            .vsync_pulse_width = 2,
            .vsync_back_porch  = 10,
            .vsync_front_porch = 10,
            .flags.pclk_active_neg = true,
        },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 1,
        .bounce_buffer_size_px = BSP_LCD_H_RES * 10,
        .hsync_gpio_num = BSP_HSYNC_GPIO,
        .vsync_gpio_num = BSP_VSYNC_GPIO,
        .de_gpio_num    = BSP_DE_GPIO,
        .pclk_gpio_num  = BSP_PCLK_GPIO,
        .data_gpio_nums = {
            BSP_B1_GPIO, BSP_B2_GPIO, BSP_B3_GPIO, BSP_B4_GPIO, BSP_B5_GPIO,
            BSP_G0_GPIO, BSP_G1_GPIO, BSP_G2_GPIO, BSP_G3_GPIO, BSP_G4_GPIO, BSP_G5_GPIO,
            BSP_R1_GPIO, BSP_R2_GPIO, BSP_R3_GPIO, BSP_R4_GPIO, BSP_R5_GPIO,
        },
        .flags.fb_in_psram = true,
    };

    /* Create ST7701S panel — the managed component wraps the RGB bus */
#if __has_include("esp_lcd_st7701.h")
    #include "esp_lcd_st7701.h"
    st7701_vendor_config_t vendor_cfg = {
        .rgb_config     = &rgb_cfg,
        .init_cmds      = NULL,    /* use component's default 480×480 init table */
        .init_cmds_size = 0,
        .flags.mirror_by_cmd    = false,
        .flags.auto_del_panel_io = true,
    };
    panel_cfg.vendor_config = &vendor_cfg;
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_st7701(s_panel_io, &panel_cfg, &s_panel), TAG, "st7701");
#else
    ESP_LOGW(TAG, "esp_lcd_st7701 component not found — creating RGB panel directly");
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_rgb_panel(&rgb_cfg, &s_panel), TAG, "rgb_panel");
#endif

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel),  TAG, "init");
    ESP_RETURN_ON_ERROR(tca9554_set_pin(TCA9554_P_LCD_CS, true), TAG, "cs_hi");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "disp_on");
    return ESP_OK;
}

/* ── Public API ──────────────────────────────────────────────────────── */
esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "TrackCluster Left BSP — ESP32-S3-Touch-LCD-2.8C");
    ESP_RETURN_ON_ERROR(i2c_and_expander_init(), TAG, "i2c");
    ESP_RETURN_ON_ERROR(panel_init(),            TAG, "panel");
    return ESP_OK;
}

static lv_disp_t *s_disp = NULL;

lv_disp_t *bsp_display_start(void)
{
    if (s_disp) return s_disp;

    const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = s_panel_io,
        .panel_handle  = s_panel,
        .buffer_size   = BSP_LCD_H_RES * 40,
        .double_buffer = false,
        .hres          = BSP_LCD_H_RES,
        .vres          = BSP_LCD_V_RES,
        .monochrome    = false,
        .color_format  = LV_COLOR_FORMAT_RGB565,
        .rotation      = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        .flags         = { .buff_dma = false, .buff_spiram = true, .swap_bytes = true },
    };
    s_disp = lvgl_port_add_disp(&disp_cfg);
    return s_disp;
}

bool bsp_lvgl_lock(uint32_t timeout_ms) { return lvgl_port_lock(timeout_ms); }
void bsp_lvgl_unlock(void)              {        lvgl_port_unlock();          }
