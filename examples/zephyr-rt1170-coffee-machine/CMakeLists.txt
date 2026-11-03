# Tier 8 example -- Qt's official Coffee Machine demo
# (qtbase/examples/demos/coffee) running on the MIMXRT1170-EVKB +
# RK055HDMIPI4MA0 panel.  QtQuick + Quick Controls 2 (Basic style) +
# SVG image resources, software scene graph (no GPU on Cortex-M7).
#
# Usage:
#   export ZEPHYR_BASE=~/org/zephyrproject/zephyrproject/zephyr
#   export QT_ZEPHYR_PREFIX=~/qt-zephyr-rt1170
#   export QT_ZEPHYR_SOURCE=$PWD/../../qt5/qtbase
#   west build -b mimxrt1170_evk@B/mimxrt1176/cm7 -p always \
#     examples/zephyr-rt1170-coffee-machine -- -DSHIELD=rk055hdmipi4ma0
#   west flash --runner linkserver
#   # press SW4 to reboot from QSPI

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qt_zephyr_rt1170_coffee_machine LANGUAGES CXX C)

set(CMAKE_PREFIX_PATH "$ENV{QT_ZEPHYR_PREFIX}" ${CMAKE_PREFIX_PATH})

set(QT_QTBASE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../..")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

set_property(TARGET Qt6::Core            PROPERTY INTERFACE_COMPILE_FEATURES "")
set_property(TARGET Qt6::Gui             PROPERTY INTERFACE_COMPILE_FEATURES "")
set_property(TARGET Qt6::Qml             PROPERTY INTERFACE_COMPILE_FEATURES "")
set_property(TARGET Qt6::Quick           PROPERTY INTERFACE_COMPILE_FEATURES "")
set_property(TARGET Qt6::QuickControls2  PROPERTY INTERFACE_COMPILE_FEATURES "")

zephyr_compile_options(-fno-short-enums)

# QEventDispatcherZephyr declares Q_OBJECT and the QML cache code emits
# additional Q_OBJECT subclasses; moc must run on every TU.
set_target_properties(app PROPERTIES AUTOMOC ON)

# Register the stock Qt 6.11 Coffee QML files as a QML module on the
# `app` target.  qt_add_qml_module produces the cache + type
# registration glue and links them statically.
qt_standard_project_setup(REQUIRES 6.6)

set(COFFEE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/coffee-src)
set_source_files_properties(
    coffee-src/Colors.qml coffee-src/Config.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(app
    URI demos.coffee
    QML_FILES
        coffee-src/ApplicationFlow.qml
        coffee-src/ApplicationFlowForm.ui.qml
        coffee-src/ChoosingCoffeeForm.ui.qml
        coffee-src/ChoosingCoffee.qml
        coffee-src/CoffeeCard.qml
        coffee-src/CoffeeCardForm.ui.qml
        coffee-src/CoffeeConfig.qml
        coffee-src/Colors.qml
        coffee-src/Config.qml
        coffee-src/CustomButton.qml
        coffee-src/CustomButtonForm.ui.qml
        coffee-src/CustomSlider.qml
        coffee-src/CustomSliderForm.ui.qml
        coffee-src/CustomToolBar.qml
        coffee-src/CustomToolBarForm.ui.qml
        coffee-src/Cup.ui.qml
        coffee-src/Home.qml
        coffee-src/HomeForm.ui.qml
        coffee-src/Insert.qml
        coffee-src/InsertForm.ui.qml
        coffee-src/Main.qml
        coffee-src/Progress.qml
        coffee-src/ProgressForm.ui.qml
        coffee-src/Ready.qml
        coffee-src/ReadyForm.ui.qml
        coffee-src/Settings.qml
        coffee-src/SettingsForm.ui.qml
    RESOURCES
        coffee-src/qtquickcontrols2.conf
        coffee-src/images/Cups/card_cup_dark.svg
        coffee-src/images/Cups/card_cup_light.svg
        coffee-src/images/Cups/dark_cup.svgz
        coffee-src/images/Cups/home_dark.svg
        coffee-src/images/Cups/home_light.svg
        coffee-src/images/Cups/light_cup.svgz
        coffee-src/images/icons/check.svg
        coffee-src/images/icons/dark_mode_black_24dp.svg
        coffee-src/images/icons/ellipse_dark.svg
        coffee-src/images/icons/ellipse_light.svg
        coffee-src/images/icons/keyboard_backspace_black.svg
        coffee-src/images/icons/keyboard_backspace_black_left.svg
        coffee-src/images/icons/keyboard_backspace_black_right.svg
        coffee-src/images/icons/keyboard_backspace_white_left.svg
        coffee-src/images/icons/keyboard_backspace_white_right.svg
        coffee-src/images/icons/light_mode_black_24dp.svg
        coffee-src/images/icons/Polygon.svg
        coffee-src/images/icons/Qt-logo-white-transparent.svg
        coffee-src/images/Ingredients/espresso_coffee.svg
        coffee-src/images/Ingredients/Milk_foam.svg
        coffee-src/images/Ingredients/milk.svg
        coffee-src/images/Ingredients/sugar.svg
)

qt_import_qml_plugins(app)

target_sources(app PRIVATE
    src/main.cpp

    # libc / Qt-internal stubs shared with earlier examples.
    src/posix_stubs.c
    src/qt_zephyr_stubs.cpp

    # Embedded TTF (Roboto-Regular, 350KB).
    src/embedded_font.c

    # Force the qzephyr plugin's symbols into the final ELF.
    src/plugin_import.cpp

    # Strong impls of the qzephyr_display_* weak hooks.
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
    "$ENV{QT_ZEPHYR_PREFIX}/include/QtQml"
    "$ENV{QT_ZEPHYR_PREFIX}/include/QtQuick"
    "${QT_PRIVATE_VERSION_DIR}"
    "${QT_PRIVATE_VERSION_DIR}/QtCore"
    "$ENV{QT_ZEPHYR_PREFIX}/include/QtGui/6.11.0/QtGui"
)

# Link the qzephyr platform plugin static archive ahead of Qt6::Gui so
# its strong symbols (QZephyrIntegrationPlugin etc.) get pulled in.
# Then the static QML plugins for the modules the Coffee demo imports
# (QtQuick / Window / Layouts / Templates / Controls / Controls.Basic)
# and the SVG image-format plugin.
target_link_libraries(app PRIVATE
    "$ENV{QT_ZEPHYR_PREFIX}/plugins/platforms/libqzephyr.a"
    # DIAGNOSTIC: bisect static QML plugins
    "$ENV{QT_ZEPHYR_PREFIX}/qml/QtQml/libqmlplugin.a"
    "$ENV{QT_ZEPHYR_PREFIX}/qml/QtQml/Models/libmodelsplugin.a"
    "$ENV{QT_ZEPHYR_PREFIX}/qml/QtQuick/libqtquick2plugin.a"
    "$ENV{QT_ZEPHYR_PREFIX}/qml/QtQuick/Window/libquickwindowplugin.a"
    "$ENV{QT_ZEPHYR_PREFIX}/qml/QtQuick/Layouts/libqquicklayoutsplugin.a"
    "$ENV{QT_ZEPHYR_PREFIX}/qml/QtQuick/Templates/libqtquicktemplates2plugin.a"
    "$ENV{QT_ZEPHYR_PREFIX}/qml/QtQuick/Controls/libqtquickcontrols2plugin.a"
    "$ENV{QT_ZEPHYR_PREFIX}/qml/QtQuick/Controls/Basic/libqtquickcontrols2basicstyleplugin.a"
    "$ENV{QT_ZEPHYR_PREFIX}/qml/QtQuick/Controls/Basic/impl/libqtquickcontrols2basicstyleimplplugin.a"
    "$ENV{QT_ZEPHYR_PREFIX}/plugins/imageformats/libqsvg.a"
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    Qt6::Core
)

zephyr_link_libraries(
    "-Wl,--unresolved-symbols=ignore-all"
    "-Wl,--warn-unresolved-symbols"
)
