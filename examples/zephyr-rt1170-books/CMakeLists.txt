# Zephyr wrapper for Qt 6.11's official "Books" SQL example on RT1170-EVKB.
# The Qt sample directory is consumed unmodified via qt_zephyr_app().
#
# Build:
#   QT_SAMPLE_DIR=$HOME/io/qt/release/Examples/Qt-6.11.1/sql/books \
#   west build -b mimxrt1170_evk@B/mimxrt1176/cm7 -p always \
#       qt5/qtbase/examples/zephyr-rt1170-books -- -DSHIELD=rk055hdmipi4ma0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE
    "LINKER:--push-state,--whole-archive"
    "<LINK_ITEM>"
    "LINKER:--pop-state")
set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qt_books LANGUAGES CXX C)

if(NOT DEFINED ENV{QT_SAMPLE_DIR})
    message(FATAL_ERROR
        "QT_SAMPLE_DIR is not set.  Point it at Qt's stock sql/books directory.")
endif()

list(APPEND CMAKE_MODULE_PATH "${ZEPHYR_QT_ZEPHYR_PORT_MODULE_DIR}/cmake")
include(QtZephyrApp)

qt_zephyr_app(
    APP_DIR $ENV{QT_SAMPLE_DIR}
    TARGET  books
)
