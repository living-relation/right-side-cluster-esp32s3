cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/esp_lvgl_port_iram_patch.cmake)
project(right_cluster)
