# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QZephyrIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QZephyrIntegrationPlugin
    OUTPUT_NAME qzephyr
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES zephyr
    STATIC
    SOURCES
        main.cpp
        qzephyrintegration.cpp qzephyrintegration.h
        qzephyrscreen.cpp qzephyrscreen.h
        qzephyrwindow.cpp qzephyrwindow.h
        qzephyrbackingstore.cpp qzephyrbackingstore.h
        qzephyr_sdlbridge.cpp qzephyr_sdlbridge.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

# OpenGL ES 2.0 through the on-target EGL (YakoGL on the AM62P).  Only
# when Stage 1 was configured with an EGL + GLES2 provider
# (build-qt-zephyr-am62p.sh); the Cortex-M7 raster build leaves this out
# so libqzephyr.a carries no egl*/gl* references there.
qt_internal_extend_target(QZephyrIntegrationPlugin CONDITION QT_FEATURE_egl AND QT_FEATURE_opengl
    SOURCES
        qzephyrglcontext.cpp qzephyrglcontext.h
    DEFINES
        QZEPHYR_WITH_EGL
    LIBRARIES
        EGL::EGL
)

# Zephyr specific libraries and includes
if(ZEPHYR_BASE)
    target_include_directories(QZephyrIntegrationPlugin PRIVATE
        ${ZEPHYR_BASE}/include
    )
endif()

# Native simulator support (SDL2-backed window + input)
option(ZEPHYR_NATIVE_SIM "Enable SDL-based backend for native_sim_64" ON)
if(ZEPHYR_NATIVE_SIM)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SDL2 IMPORTED_TARGET sdl2)
        if(TARGET PkgConfig::SDL2)
            target_link_libraries(QZephyrIntegrationPlugin PRIVATE PkgConfig::SDL2)
            target_compile_definitions(QZephyrIntegrationPlugin PRIVATE QZEPHYR_WITH_SDL)
        endif()
    endif()
endif()
