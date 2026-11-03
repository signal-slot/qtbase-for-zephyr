# Copyright (C) 2026 Signal Slot Inc.
# SPDX-License-Identifier: BSD-3-Clause

# wolfSSL install prefix — set by the build script
if(NOT WOLFSSL_PREFIX)
    if(DEFINED ENV{WOLFSSL_PREFIX})
        set(WOLFSSL_PREFIX "$ENV{WOLFSSL_PREFIX}")
    endif()
endif()

if(NOT WOLFSSL_PREFIX OR NOT EXISTS "${WOLFSSL_PREFIX}/lib/libwolfssl.a")
    # Try lib64 (host build)
    if(WOLFSSL_PREFIX AND EXISTS "${WOLFSSL_PREFIX}/lib64/libwolfssl.a")
        set(_wolfssl_lib "${WOLFSSL_PREFIX}/lib64/libwolfssl.a")
    else()
        message(STATUS "wolfSSL not found (WOLFSSL_PREFIX=${WOLFSSL_PREFIX}), skipping wolfssl TLS plugin")
        return()
    endif()
else()
    set(_wolfssl_lib "${WOLFSSL_PREFIX}/lib/libwolfssl.a")
endif()

set(_wolfssl_inc "${WOLFSSL_PREFIX}/include")
if(NOT EXISTS "${_wolfssl_inc}/wolfssl/ssl.h")
    message(STATUS "wolfSSL headers not found at ${_wolfssl_inc}, skipping wolfssl TLS plugin")
    return()
endif()

# user_settings_qt.h lives alongside the wolfSSL source.
# The build script copies it into the install prefix/include/ during build.
# We also need WOLFSSL_USER_SETTINGS so wolfSSL headers pick up user_settings.h.
if(EXISTS "${_wolfssl_inc}/user_settings.h")
    set(_wolfssl_user_settings_def "WOLFSSL_USER_SETTINGS")
else()
    set(_wolfssl_user_settings_def "")
endif()

qt_internal_add_plugin(QTlsBackendWolfSSLPlugin
    OUTPUT_NAME qwolfsslbackend
    CLASS_NAME QTlsBackendWolfSSL
    PLUGIN_TYPE tls
    SOURCES
        ../shared/qx509_base_p.h
        ../shared/qx509_base.cpp
        ../shared/qx509_generic_p.h
        ../shared/qx509_generic.cpp
        ../shared/qasn1element_p.h
        ../shared/qasn1element.cpp
        ../shared/qtlskey_base_p.h
        ../shared/qtlskey_base.cpp
        ../shared/qtlskey_generic_p.h
        ../shared/qtlskey_generic.cpp
        qtlsbackend_wolfssl_p.h
        qtlsbackend_wolfssl.cpp
        qtls_wolfssl_p.h
        qtls_wolfssl.cpp
    LIBRARIES
        Qt::NetworkPrivate
    DEFINES
        QT_NO_CAST_FROM_ASCII
        ${_wolfssl_user_settings_def}
)

target_include_directories(QTlsBackendWolfSSLPlugin PRIVATE "${_wolfssl_inc}")
target_link_libraries(QTlsBackendWolfSSLPlugin PRIVATE "${_wolfssl_lib}")
