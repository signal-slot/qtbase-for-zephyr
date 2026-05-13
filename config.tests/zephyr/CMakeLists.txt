cmake_minimum_required(VERSION 3.20)
project(qt_config_test_zephyr LANGUAGES C CXX)

# Test if we're building for Zephyr
if(NOT DEFINED ZEPHYR_BASE)
    message(FATAL_ERROR "ZEPHYR_BASE not defined. Zephyr SDK is not properly configured.")
endif()

# Test if Zephyr SDK is available
if(NOT DEFINED ZEPHYR_SDK_INSTALL_DIR AND NOT DEFINED ENV{ZEPHYR_SDK_INSTALL_DIR})
    message(FATAL_ERROR "ZEPHYR_SDK_INSTALL_DIR not defined. Please install Zephyr SDK.")
endif()

# Check for Zephyr toolchain
if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "C compiler not found. Zephyr toolchain not properly configured.")
endif()

if(NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "C++ compiler not found. Zephyr toolchain not properly configured.")
endif()

# Try to compile a simple test program
add_executable(zephyr_test main.cpp)

# Set minimal compilation flags for Zephyr
target_compile_options(zephyr_test PRIVATE
    -ffreestanding
    -fno-common
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
)

# Minimal link flags
set_target_properties(zephyr_test PROPERTIES
    LINK_FLAGS "-nostdlib -nodefaultlibs"
)

# If we get here, the test passed
message(STATUS "Zephyr SDK configuration test passed")