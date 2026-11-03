# Qt Platform Plugin for Zephyr RTOS

This directory contains the Qt platform abstraction plugin for Zephyr RTOS.

## Overview

The Zephyr platform plugin provides the minimal implementation required to run Qt applications on Zephyr RTOS. It implements:

- **QZephyrIntegration**: The main platform integration class
- **QZephyrScreen**: Screen/display management
- **QZephyrWindow**: Window management (typically single window for embedded)
- **QZephyrBackingStore**: Software rendering backend

## Building

To build Qt with Zephyr support:

1. Set up Zephyr SDK and environment:
   ```bash
   export ZEPHYR_SDK_INSTALL_DIR=/path/to/zephyr-sdk
   export ZEPHYR_BASE=/home/tasuku/org/zephyrproject/zephyr
   ```

2. Configure Qt with Zephyr platform:
   ```bash
   cmake -GNinja \
     -DCMAKE_SYSTEM_NAME=Zephyr \
     -DQT_QMAKE_TARGET_MKSPEC=zephyr-arm-gnueabi-g++ \
     -DQT_HOST_PATH=/path/to/host/qt \
     -DCMAKE_INSTALL_PREFIX=/path/to/install \
     ..
   ```

## Features

- Software rendering via QPainter
- Single window/fullscreen applications
- Minimal memory footprint
- No dynamic library support (static linking only)

## Limitations

- No OpenGL support
- No multi-window support
- No clipboard functionality
- No native file dialogs
- Limited font support

## TODO

- Integrate with Zephyr display driver API
- Add input device support (touch, buttons)
- Implement proper event loop integration
- Add configuration options for display parameters
- Support for different display controllers