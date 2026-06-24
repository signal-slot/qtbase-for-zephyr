// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QZEPHYRBACKINGSTORE_H
#define QZEPHYRBACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>
#include <QtGui/qimage.h>
#ifdef QZEPHYR_WITH_SDL
#include "qzephyr_sdlbridge.h"
#endif

QT_BEGIN_NAMESPACE

class QZephyrBackingStore : public QPlatformBackingStore
{
public:
    QZephyrBackingStore(QWindow *window);
    ~QZephyrBackingStore();

    QPaintDevice *paintDevice() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;

private:
    QImage m_image;
    // Optional GC355 vg_lite paint device wrapping m_image's buffer, created via
    // the weak Stage-2 hook qzephyr_make_vglite_paint_device() when
    // CONFIG_QT_VGLITE_QPAINTENGINE is built in; null -> plain QImage raster.
    QPaintDevice *m_vgliteDevice = nullptr;
};

QT_END_NAMESPACE

#endif // QZEPHYRBACKINGSTORE_H
