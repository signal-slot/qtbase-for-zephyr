// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QZEPHYRSCREEN_H
#define QZEPHYRSCREEN_H

#include <qpa/qplatformscreen.h>
#include <QtGui/qimage.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class QZephyrWindow;

class QZephyrScreen : public QPlatformScreen
{
public:
    QZephyrScreen();
    ~QZephyrScreen();

    QRect geometry() const override { return m_geometry; }
    int depth() const override { return m_depth; }
    QImage::Format format() const override { return m_format; }
    QSizeF physicalSize() const override { return m_physicalSize; }

    void addWindow(QZephyrWindow *w);
    void removeWindow(QZephyrWindow *w);
    bool hasVisiblePopups() const;
    const QImage &composite();

private:
    QRect m_geometry;
    int m_depth;
    QImage::Format m_format;
    QSizeF m_physicalSize;
    QImage m_compositeBuf;
    QList<QZephyrWindow *> m_windows;
};

QT_END_NAMESPACE

#endif // QZEPHYRSCREEN_H