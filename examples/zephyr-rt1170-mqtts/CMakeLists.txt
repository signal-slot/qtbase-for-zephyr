# MQTTS (MQTT over TLS) on RT1170-EVKB
#
# Build:
#   west build -b mimxrt1170_evk@B/mimxrt1176/cm7 -p always \
#       qt5/qtbase/examples/zephyr-rt1170-mqtts -- -DSHIELD=rk055hdmipi4ma0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE
    "LINKER:--push-state,--whole-archive"
    "<LINK_ITEM>"
    "LINKER:--pop-state")
set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qt_mqtts LANGUAGES CXX C)

list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_ZEPHYR_PREFIX}")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

target_sources(app PRIVATE src/main.cpp)
target_link_libraries(app PRIVATE
    Qt6::Network
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
)
