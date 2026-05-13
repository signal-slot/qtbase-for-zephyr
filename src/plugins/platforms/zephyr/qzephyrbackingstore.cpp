// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qzephyrbackingstore.h"
#include "qzephyrscreen.h"
#include <qpa/qplatformscreen.h>
#include <QtGui/qpainter.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QZephyrBackingStore::QZephyrBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
}

QZephyrBackingStore::~QZephyrBackingStore()
{
}

QPaintDevice *QZephyrBackingStore::paintDevice()
{
    return &m_image;
}

void QZephyrBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);
    
    if (m_image.size() == size)
        return;
        
    QImage::Format format = QImage::Format_RGB16;  // Default format for embedded systems
    if (window()->screen()) {
        QZephyrScreen *screen = static_cast<QZephyrScreen *>(window()->screen()->handle());
        if (screen)
            format = screen->format();
    }
    
    m_image = QImage(size, format);
    m_image.fill(Qt::black);
}

// Weak hook supplied by the Stage 2 Zephyr application -- see
// examples/zephyr-rt1170-gui/src/qzephyr_display_zephyr.cpp.  When the
// app provides a strong definition the non-SDL branch of flush() routes
// per-rect writes through Zephyr's <zephyr/drivers/display.h>
// display_write() API.  Stage 1 cannot include <zephyr/...> so the call
// site stays a weak extern; if no strong impl is linked the pointer is
// null and we fall back to a qDebug.
extern "C" __attribute__((weak))
void qzephyr_display_write(int x, int y, int w, int h,
                           int pitch_bytes, int bytes_per_pixel,
                           const void *buf);

void QZephyrBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(offset);

    if (m_image.isNull())
        return;

#ifdef QZEPHYR_WITH_SDL
    // SDL-backed native_sim_64 path: upload the entire buffer for simplicity
    extern void qzephyr_sdl_present(const QImage &img, const QRegion &region);
    qzephyr_sdl_present(m_image, region.isEmpty() ? QRegion(QRect(QPoint(0, 0), m_image.size())) : region);
#else
    Q_UNUSED(window);
    Q_UNUSED(region);
    if (qzephyr_display_write) {
        // Whole-frame flush regardless of dirty region: the only known
        // production path is the RT1170 LCDIF + PXP pipeline, and that
        // pipeline requires whole-frame source writes when configured
        // for hardware rotation (CONFIG_MCUX_ELCDIF_PXP_ROTATE_*).
        // Passing a sub-rect corrupts the rotated framebuffer on every
        // call after the first.  Bandwidth-wise this is fine: a 1280x720
        // RGB16 frame is ~1.84 MB and the panel only refreshes at 60 Hz.
        const int bpp = m_image.depth() / 8;
        const int pitch = m_image.bytesPerLine();
        qzephyr_display_write(0, 0, m_image.width(), m_image.height(),
                              pitch, bpp, m_image.constBits());
    } else {
        qDebug() << "QZephyrBackingStore::flush (no qzephyr_display_write hook) region:"
                 << region.boundingRect();
    }
#endif
}

QT_END_NAMESPACE
