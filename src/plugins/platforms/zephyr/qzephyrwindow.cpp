// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qzephyrwindow.h"
#include "qzephyrscreen.h"
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
    if (isPopup()) {
        m_geometry = window->geometry();
    } else {
        m_geometry = screen() ? screen()->geometry() : window->geometry();
    }

    auto *scr = static_cast<QZephyrScreen *>(screen());
    if (scr)
        scr->addWindow(this);
}

QZephyrWindow::~QZephyrWindow()
{
    auto *scr = static_cast<QZephyrScreen *>(screen());
    if (scr)
        scr->removeWindow(this);
}

void QZephyrWindow::setGeometry(const QRect &rect)
{
    QRect newGeom;
    if (isPopup()) {
        // Qt resets popup position to (0,0) after hide; ignore geometry
        // changes while hidden — the correct position from showPopup()
        // is already cached from the last visible setGeometry call.
        if (!m_visible)
            return;
        newGeom = rect;
    } else {
        newGeom = screen() ? screen()->geometry() : m_geometry;
    }

    if (m_geometry == newGeom)
        return;

    m_geometry = newGeom;
    QPlatformWindow::setGeometry(m_geometry);

    QWindowSystemInterface::handleGeometryChange(window(), m_geometry);
    QWindowSystemInterface::handleExposeEvent(window(),
                                              QRect(QPoint(0, 0), m_geometry.size()));
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
        QWindowSystemInterface::handleExposeEvent(window(),
                                                  QRect(QPoint(0, 0), m_geometry.size()));
    } else {
        QWindowSystemInterface::handleExposeEvent(window(), QRegion());
    }
}

bool QZephyrWindow::isPopup() const
{
    const Qt::WindowType type = window()->type();
    return type == Qt::Popup || type == Qt::ToolTip || type == Qt::Tool;
}

QT_END_NAMESPACE
