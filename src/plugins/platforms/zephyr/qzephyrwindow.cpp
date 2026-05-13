// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qzephyrwindow.h"
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformscreen.h>
#include <QtGui/qwindow.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

static WId s_nextWindowId = 1;

QZephyrWindow::QZephyrWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_visible(false)
    , m_winId(s_nextWindowId++)
{
    // Kiosk-style platform: every top-level window fills the single
    // panel.  Ignore the QWindow's requested geometry (e.g. a QDialog's
    // sizeHint) and always expand to the screen's full landscape rect
    // so the QPainter backbuffer matches the panel resolution, and any
    // hardware path that requires whole-frame writes (e.g. PXP rotation)
    // sees a canvas of the right size.
    m_geometry = screen() ? screen()->geometry() : window->geometry();
}

QZephyrWindow::~QZephyrWindow()
{
}

void QZephyrWindow::setGeometry(const QRect &rect)
{
    // Kiosk-style: ignore caller-supplied rect; the panel is the only
    // canvas and is always full-screen.  This avoids partial-screen
    // QDialog / QWidget layouts that the panel's PXP rotation cannot
    // handle (partial updates corrupt the rotated framebuffer).
    Q_UNUSED(rect);
    const QRect screenRect = screen() ? screen()->geometry() : m_geometry;
    if (m_geometry == screenRect)
        return;

    m_geometry = screenRect;
    QPlatformWindow::setGeometry(m_geometry);

    QWindowSystemInterface::handleGeometryChange(window(), m_geometry);
    QWindowSystemInterface::handleExposeEvent(window(), m_geometry);
}

QRect QZephyrWindow::geometry() const
{
    return m_geometry;
}

WId QZephyrWindow::winId() const
{
    return m_winId;
}

void QZephyrWindow::setVisible(bool visible)
{
    if (m_visible == visible)
        return;
        
    m_visible = visible;
    
    if (visible) {
        QWindowSystemInterface::handleExposeEvent(window(), m_geometry);
    } else {
        QWindowSystemInterface::handleExposeEvent(window(), QRegion());
    }
}

QT_END_NAMESPACE