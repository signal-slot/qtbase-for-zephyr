cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE
    "LINKER:--push-state,--whole-archive"
    "<LINK_ITEM>"
    "LINKER:--pop-state")
set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)

# Zephyr's toolchain file does not populate CMAKE_CXX_COMPILE_FEATURES;
# Qt6::Platform's INTERFACE_COMPILE_FEATURES needs cxx_std_17.
set(CMAKE_CXX_COMPILE_FEATURES "cxx_std_17" CACHE INTERNAL "")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qt_audio_test LANGUAGES CXX C)

set(ZEPHYR_QT_ZEPHYR_PORT_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../zephyr-module")
list(APPEND CMAKE_MODULE_PATH "${ZEPHYR_QT_ZEPHYR_PORT_MODULE_DIR}/cmake")
include(QtZephyrApp)

qt_zephyr_app(
    APP_DIR ${CMAKE_CURRENT_LIST_DIR}/src
    TARGET  audio_test
)
