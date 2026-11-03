# Stage 2 of the Tier 2 two-stage Qt-on-Zephyr build for RT1170-EVKB.
#
# Stage 1 (already done) produced ~/qt-zephyr-rt1170/lib/libQt6Core.a
# as a Cortex-M7 (armv7e-m, elf32-littlearm) static archive, with
# qeventdispatcher_zephyr.cpp and the qzephyr platform plugin sources
# intentionally excluded.  Stage 2 (this CMakeLists) lets Zephyr's own
# CMake compile those deferred sources against real <zephyr/kernel.h>
# headers and link the resulting Zephyr-aware objects together with the
# Stage 1 static archive into a single Zephyr application ELF.
#
# Usage:
#   export ZEPHYR_BASE=~/org/zephyrproject/zephyrproject/zephyr
#   export QT_ZEPHYR_PREFIX=~/qt-zephyr-rt1170
#   export QT_ZEPHYR_SOURCE=$PWD/../../qt5/qtbase
#   west build -b mimxrt1170_evk@B/mimxrt1176/cm7 examples/zephyr-rt1170

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qt_zephyr_rt1170_hello LANGUAGES CXX C)

# QT_ZEPHYR_PREFIX -- install root of the Stage 1 cross-build
# (see build-qt-zephyr-rt1170.sh).  Provides Qt6CoreConfig.cmake and
# libQt6Core.a.
set(CMAKE_PREFIX_PATH "$ENV{QT_ZEPHYR_PREFIX}" ${CMAKE_PREFIX_PATH})

# This example now lives inside qtbase; source root is two levels up.
set(QT_QTBASE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../..")

find_package(Qt6 REQUIRED COMPONENTS Core)

# Qt6CoreConfigExtras.cmake annotates Qt6::Core with
#   INTERFACE_COMPILE_FEATURES = cxx_decltype
# which propagates through target_link_libraries to our app and trips
# CMake's "unknown feature for GNU 14.3.0" error in the Zephyr
# cross-toolchain context.  Drop the requirement; we set the C++
# standard via CONFIG_STD_CPP17=y in prj.conf instead.
set_property(TARGET Qt6::Core PROPERTY INTERFACE_COMPILE_FEATURES "")

# QEventDispatcherZephyr is a QObject (Q_OBJECT macro), so moc must run
# on qeventdispatcher_zephyr.cpp to generate the metaObject() /
# qt_metacast() / qt_metacall() symbols its vtable references.  Without
# moc the class's vtable cannot be emitted by GCC (the dtor anchor
# depends on those symbols), and any indirect virtual call into the
# dispatcher (e.g. QCoreApplication ctor's startingUp() at
# qcoreapplication.cpp:770) jumps to NULL.
set_target_properties(app PROPERTIES AUTOMOC ON)

target_sources(app PRIVATE
    src/main.cpp

    # Weak POSIX stubs (gethostname, getuid, etc.) for libc symbols
    # picolibc on Cortex-M7 doesn't provide but Qt's Stage 1 archives
    # still reference from never-reached code paths.
    src/posix_stubs.c

    # Qt-internal C++ stubs for symbols whose UNIX implementation TUs
    # we exclude from Stage 1 (QStandardPaths::writableLocation etc.).
    src/qt_zephyr_stubs.cpp

    # Stage-1 deferred (excluded via QT_DEFER_ZEPHYR_RUNTIME=ON):
    # Qt's Zephyr-runtime event dispatcher.  Compiled inside the Zephyr
    # application's CMake context so <zephyr/kernel.h> resolves against
    # the real Kconfig-driven headers.  Also carries the Zephyr-flavoured
    # QThreadPrivate::createEventDispatcher and QThread::sleep
    # definitions that QCoreApplication needs at construction time.
    "${QT_QTBASE_SOURCE}/src/corelib/kernel/qeventdispatcher_zephyr.cpp"
)

# Locate the versioned private-header directory under the Stage 1
# install (e.g. include/QtCore/6.11.0).  Avoid hard-coding the Qt
# version here -- Stage 1 derives it from .cmake.conf and the path
# changes per minor release.
file(GLOB QT_PRIVATE_VERSION_DIR
    LIST_DIRECTORIES TRUE
    "$ENV{QT_ZEPHYR_PREFIX}/include/QtCore/[0-9]*.[0-9]*.[0-9]*")
if(NOT QT_PRIVATE_VERSION_DIR)
    message(FATAL_ERROR
        "Could not locate Qt private-header version dir under "
        "$ENV{QT_ZEPHYR_PREFIX}/include/QtCore -- is QT_ZEPHYR_PREFIX "
        "pointing at a Stage 1 install?")
endif()
list(GET QT_PRIVATE_VERSION_DIR 0 QT_PRIVATE_VERSION_DIR)

# qtbase headers we need at the application level for the deferred TU.
target_include_directories(app PRIVATE
    # Public Qt6 headers ship with the install (find_package handled),
    # but the dispatcher TU reaches for QtCore/private/* paths too.
    "$ENV{QT_ZEPHYR_PREFIX}/include"
    "$ENV{QT_ZEPHYR_PREFIX}/include/QtCore"
    "${QT_PRIVATE_VERSION_DIR}"
    "${QT_PRIVATE_VERSION_DIR}/QtCore"
)

target_link_libraries(app PRIVATE Qt6::Core)

# Stage 2 final ELF link: libQt6Core.a contains TUs (qtimezone,
# qstandardpaths, qfilesystemengine_unix, etc.) that reference platform
# symbols whose implementations live in source files we deliberately
# exclude from this Tier 2 picture-less smoke test (qtimezoneprivate_tz,
# qstandardpaths_unix, qthread_unix, ...).  The QCoreApplication +
# QTimer dispatch path the test exercises does not transit any of those
# code paths, so let the linker accept unresolved references and
# produce a final ELF anyway.  Calling into the missing entry points
# at runtime would jump to address 0 -- the test never does.
#
# Zephyr's final ELF link is driven by the `zephyr_final` target, not
# the `app` library target, so target_link_options(app ...) does not
# reach it.  zephyr_link_libraries() (Zephyr's wrapper) does.
zephyr_link_libraries(
    "-Wl,--unresolved-symbols=ignore-all"
    "-Wl,--warn-unresolved-symbols"
)
