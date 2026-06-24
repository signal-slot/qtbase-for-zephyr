# Qt + FAT filesystem on SD card test for RT1170-EVKB
#
# Build:
#   west build -b mimxrt1170_evk@B/mimxrt1176/cm7 -p always \
#       qt5/qtbase/examples/zephyr-rt1170-fatfs-qt -- -DSHIELD=rk055hdmipi4ma0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE
    "LINKER:--push-state,--whole-archive"
    "<LINK_ITEM>"
    "LINKER:--pop-state")
set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fatfs_qt LANGUAGES CXX C)

set(CMAKE_CXX_COMPILE_FEATURES cxx_std_17 CACHE STRING "" FORCE)

list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_ZEPHYR_PREFIX}")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

target_sources(app PRIVATE src/main.cpp)
target_link_libraries(app PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
)
