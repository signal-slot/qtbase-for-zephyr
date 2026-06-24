# SQLite in-memory test on RT1170-EVKB
#
# Build:
#   west build -b mimxrt1170_evk@B/mimxrt1176/cm7 -p always \
#       qt5/qtbase/examples/zephyr-rt1170-sqlite -- -DSHIELD=rk055hdmipi4ma0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE
    "LINKER:--push-state,--whole-archive"
    "<LINK_ITEM>"
    "LINKER:--pop-state")
set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qt_sqlite LANGUAGES CXX C)

list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_ZEPHYR_PREFIX}")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

target_sources(app PRIVATE src/main.cpp src/posix_stubs.c src/sqlite_vfs_patch.c)
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/3rdparty/sqlite
)
target_link_libraries(app PRIVATE
    Qt6::Sql
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
)

# Wrap lseek/read to fix FatFS's refusal to seek beyond EOF (POSIX requires it)
zephyr_link_libraries(-Wl,--wrap=lseek,--wrap=read)

