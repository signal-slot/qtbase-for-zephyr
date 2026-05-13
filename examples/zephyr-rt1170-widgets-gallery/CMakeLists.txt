# Tier 6 example -- Qt's official Widgets Gallery (from
# qtbase/examples/widgets/gallery) running on the MIMXRT1170-EVKB +
# RK055HDMIPI4MA0 panel.
#
# Usage:
#   export ZEPHYR_BASE=~/org/zephyrproject/zephyrproject/zephyr
#   export QT_ZEPHYR_PREFIX=~/qt-zephyr-rt1170
#   export QT_ZEPHYR_SOURCE=$PWD/../../qt5/qtbase
#   west build -b mimxrt1170_evk@B/mimxrt1176/cm7 -p always \
#     examples/zephyr-rt1170-widgets-gallery -- -DSHIELD=rk055hdmipi4ma0
#   west flash --runner linkserver
#   # press SW4 to reboot from QSPI

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qt_zephyr_rt1170_widgets_gallery LANGUAGES CXX C)

set(CMAKE_PREFIX_PATH "$ENV{QT_ZEPHYR_PREFIX}" ${CMAKE_PREFIX_PATH})

# This example lives inside qtbase; the source root is two levels up.
set(QT_QTBASE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../..")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set_property(TARGET Qt6::Core    PROPERTY INTERFACE_COMPILE_FEATURES "")
set_property(TARGET Qt6::Gui     PROPERTY INTERFACE_COMPILE_FEATURES "")
set_property(TARGET Qt6::Widgets PROPERTY INTERFACE_COMPILE_FEATURES "")

# Stage 1 was built with -fno-short-enums; match it across the app TUs.
zephyr_compile_options(-fno-short-enums)

# QEventDispatcherZephyr and WidgetGallery both declare Q_OBJECT -- moc
# must run on each compilation unit so the vtable's metaObject() /
# qt_metacast() / qt_metacall() anchors get emitted.
set_target_properties(app PROPERTIES AUTOMOC ON)

target_sources(app PRIVATE
    src/main.cpp

    # Qt's stock WidgetGallery widget tree (lifted verbatim from
    # qtbase/examples/widgets/gallery/widgetgallery.{h,cpp}).  AUTOMOC
    # generates the moc TU for the Q_OBJECT class.
    src/widgetgallery.cpp

    # libc / Qt-internal stubs shared with earlier Tier 2/3 examples.
    src/posix_stubs.c
    src/qt_zephyr_stubs.cpp

    # Embedded TTF (Roboto-Regular, 350KB) -- main.cpp registers it
    # via QFontDatabase::addApplicationFontFromData.
    src/embedded_font.c

    # Force the qzephyr plugin's QZephyrIntegrationPlugin symbols out
    # of libqzephyr.a into the final ELF, and pre-set
    # QT_QPA_PLATFORM=zephyr before main().
    src/plugin_import.cpp

    # Strong impls of the qzephyr_display_* weak hooks declared in
    # qzephyrbackingstore.cpp / qzephyrscreen.cpp.  Pulls in
    # <zephyr/drivers/display.h>.
    src/qzephyr_display_zephyr.cpp

    # GT911 capacitive touch -> QWindowSystemInterface mouse events.
    src/qzephyr_touch_zephyr.cpp

    # Stage-1 deferred Zephyr-runtime event dispatcher.
    "${QT_QTBASE_SOURCE}/src/corelib/kernel/qeventdispatcher_zephyr.cpp"
)

file(GLOB QT_PRIVATE_VERSION_DIR
    LIST_DIRECTORIES TRUE
    "$ENV{QT_ZEPHYR_PREFIX}/include/QtCore/[0-9]*.[0-9]*.[0-9]*")
list(GET QT_PRIVATE_VERSION_DIR 0 QT_PRIVATE_VERSION_DIR)

target_include_directories(app PRIVATE
    "$ENV{QT_ZEPHYR_PREFIX}/include"
    "$ENV{QT_ZEPHYR_PREFIX}/include/QtCore"
    "$ENV{QT_ZEPHYR_PREFIX}/include/QtGui"
    "$ENV{QT_ZEPHYR_PREFIX}/include/QtWidgets"
    "${QT_PRIVATE_VERSION_DIR}"
    "${QT_PRIVATE_VERSION_DIR}/QtCore"
    # qpa/* private headers used by the touch hook.
    "$ENV{QT_ZEPHYR_PREFIX}/include/QtGui/6.11.0/QtGui"
)

# Link the qzephyr platform plugin static archive ahead of Qt6::Gui so
# its strong symbols (QZephyrIntegrationPlugin etc.) get pulled in.
# Order: Widgets -> Gui -> Core for symbol resolution.
target_link_libraries(app PRIVATE
    "$ENV{QT_ZEPHYR_PREFIX}/plugins/platforms/libqzephyr.a"
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
)

zephyr_link_libraries(
    "-Wl,--unresolved-symbols=ignore-all"
    "-Wl,--warn-unresolved-symbols"
)
