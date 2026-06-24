# FAT filesystem on SD card smoke test for RT1170-EVKB
#
# Build:
#   west build -b mimxrt1170_evk@B/mimxrt1176/cm7 -p always \
#       qt5/qtbase/examples/zephyr-rt1170-fatfs

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fatfs_test LANGUAGES C)

target_sources(app PRIVATE src/main.c)
