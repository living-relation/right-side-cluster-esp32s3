include(${CMAKE_CURRENT_LIST_DIR}/../cmake/esp_lvgl_port_iram_patch.cmake)

idf_component_register(
    SRCS
        "main.c"
        "bench_demo.c"
        "bsp.c"
        "uart_rx.c"
        "alarm_task.c"
        "dash_data.c"
        "ui/ui.c"
        "ui/ui_lambda.c"
        "ui/ui_bar_gauges.c"
        "ui/ui_menu_popup.c"
        "ui/ui_warning.c"
        "ui/ui_boot.c"
        "ui/ui_numeric.c"
        "ui/ui_signal_filter.c"
        "ui/fonts/aerospace_56.c"
        "ui/fonts/aerospace_22.c"
        "ui/fonts/racehead_17.c"
        "ui/fonts/racehead_19.c"
        "ui/fonts/racehead_22.c"
        "ui/fonts/racehead_14.c"
        "ui/fonts/racehead_12.c"
        "ui/assets/toyota_logo.c"
        "ui/assets/lambda_64.c"

    INCLUDE_DIRS
        "."
        "ui"

    PRIV_REQUIRES
        driver
        esp_lcd
        esp_driver_gpio
        esp_driver_uart
        esp_driver_i2c
        esp_psram
        nvs_flash
)

target_compile_definitions(__idf_main PRIVATE LV_LVGL_H_INCLUDE_SIMPLE=1)
