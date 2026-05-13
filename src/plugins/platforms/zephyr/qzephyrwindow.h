// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QZEPHYRWINDOW_H
#define QZEPHYRWINDOW_H

#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

class QZephyrWindow : public QPlatformWindow
{
public:
    QZephyrWindow(QWindow *window);
    ~QZephyrWindow();

    void setGeometry(const QRect &rect) override;
    QRect geometry() const override;
    
    WId winId() const override;
    void setVisible(bool visible) override;

private:
    QRect m_geometry;
    bool m_visible;
    WId m_winId;
};

QT_END_NAMESPACE

#endif // QZEPHYRWINDOW_H