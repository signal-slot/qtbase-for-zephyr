# Minimal Zephyr wrapper that boots Qt 6.11's *unmodified* official
# Coffee Machine demo (qtbase/examples/demos/coffee) on an RT1170-EVKB.
#
# The wrapper carries ONLY:
#   1. find_package(Zephyr) -- Zephyr's build entry-point.
#   2. project()             -- standard CMake.
#   3. qt_zephyr_app(...)    -- the Qt-on-Zephyr module's helper.
#
# Everything else (board overlay, prj.conf, QPA glue, font, plugin
# imports) lives in qt5/zephyr-module.  The Qt sample directory is
# pointed at via QT_SAMPLE_DIR -- its main.cpp / CMakeLists.txt /
# *.qml are touched ZERO times.
#
# Build:
#   QT_SAMPLE_DIR=$HOME/io/qt/release/Examples/Qt-6.11.0/demos/coffee \
#   QT_ZEPHYR_PREFIX=$HOME/qt-zephyr-rt1170                            \
#   ZEPHYR_BASE=$HOME/org/zephyrproject/zephyrproject/zephyr           \
#   ZEPHYR_EXTRA_MODULES=$PWD/../../zephyr-module                      \
#   west build -b mimxrt1170_evk@B/mimxrt1176/cm7 -p always            \
#       examples/zephyr-rt1170-coffee-as-is -- -DSHIELD=rk055hdmipi4ma0

cmake_minimum_required(VERSION 3.20.0)

# CMake 4.x no longer auto-detects WHOLE_ARCHIVE support for the CXX link
# language under arm-zephyr-eabi; declare it before find_package(Zephyr) so
# qt_zephyr_app()'s $<LINK_LIBRARY:WHOLE_ARCHIVE,...> resolves.
set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE
    "LINKER:--push-state,--whole-archive"
    "<LINK_ITEM>"
    "LINKER:--pop-state")
set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qt_coffee_as_is LANGUAGES CXX C)

if(NOT DEFINED ENV{QT_SAMPLE_DIR})
    message(FATAL_ERROR
        "QT_SAMPLE_DIR is not set.  Point it at Qt's stock demos/coffee "
        "directory (the one that ships in the Qt SDK -- it must not be "
        "modified).")
endif()

# Make the qt-zephyr-port module's cmake/ dir discoverable.
# (Zephyr exposes per-module roots as ZEPHYR_<NAME>_MODULE_DIR.)
list(APPEND CMAKE_MODULE_PATH "${ZEPHYR_QT_ZEPHYR_PORT_MODULE_DIR}/cmake")
include(QtZephyrApp)

qt_zephyr_app(
    APP_DIR $ENV{QT_SAMPLE_DIR}
    TARGET  coffeemachine
)
