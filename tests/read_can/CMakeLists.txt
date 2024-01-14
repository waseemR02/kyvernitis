cmake_minimum_required(VERSION 3.13.1)
set(BOARD blackpill_f401cc)
set(DTC_OVERLAY_FILE "boards/app.overlay")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(kyvernitis_tests LANGUAGES C)

target_sources(app PRIVATE src/main.c)
