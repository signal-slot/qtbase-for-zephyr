// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qzephyrscreen.h"
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

// Tier 3-3 hook supplied by the Stage 2 Zephyr application -- see
// examples/zephyr-rt1170-gui/src/qzephyr_display_zephyr.cpp.  Returns
// the panel resolution and Qt-side QImage format matching the driver's
// reported display_pixel_format.  Stage 1 cannot include
// <zephyr/drivers/display.h>, so this is a weak extern.  If no strong
// impl is linked (host SDL2 sim, or app forgot to register), env-var
// defaults below kick in.
extern "C" __attribute__((weak))
bool qzephyr_display_query_caps(int *w, int *h, int *qimage_format);

QZephyrScreen::QZephyrScreen()
{
    int w = 0, h = 0, fmt_int = 0;
    bool fromDriver = false;
    if (qzephyr_display_query_caps
        && qzephyr_display_query_caps(&w, &h, &fmt_int)) {
        m_format = static_cast<QImage::Format>(fmt_int);
        fromDriver = true;
    } else {
        // Host SDL simulator or unhooked build: env-var defaults.
        w = qEnvironmentVariableIntValue("QT_ZEPHYR_WIDTH") ? qgetenv("QT_ZEPHYR_WIDTH").toInt() : 800;
        h = qEnvironmentVariableIntValue("QT_ZEPHYR_HEIGHT") ? qgetenv("QT_ZEPHYR_HEIGHT").toInt() : 480;

        const QByteArray fmt = qgetenv("QT_ZEPHYR_FORMAT").toLower();
        if (fmt == "rgb888")
            m_format = QImage::Format_RGB888;
        else
            m_format = QImage::Format_RGB16;
    }

    switch (m_format) {
    case QImage::Format_RGB888:    m_depth = 24; break;
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGB32:     m_depth = 32; break;
    case QImage::Format_RGB16:
    default:                       m_depth = 16; break;
    }

    m_geometry = QRect(0, 0, w, h);
    // Approximate 100 DPI
    m_physicalSize = QSizeF(w / 3.78, h / 3.78);

    qDebug() << "QZephyrScreen: geometry" << m_geometry
             << "format" << m_format << "depth" << m_depth
             << (fromDriver ? "(from driver)" : "(env default)");
}

QZephyrScreen::~QZephyrScreen()
{
}

QT_END_NAMESPACE
