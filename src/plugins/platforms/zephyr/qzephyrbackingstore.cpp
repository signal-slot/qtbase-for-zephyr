// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qzephyrbackingstore.h"
#include "qzephyrscreen.h"
#include "qzephyrwindow.h"
#include <qpa/qplatformscreen.h>
#include <QtGui/qpainter.h>
#include <QtGui/qwindow.h>
#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>

QT_BEGIN_NAMESPACE

QZephyrBackingStore::QZephyrBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
}

QZephyrBackingStore::~QZephyrBackingStore()
{
    delete m_vgliteDevice;
}

// Weak Stage-2 hook (CONFIG_QT_VGLITE_QPAINTENGINE) -- returns a QPaintDevice
// whose paintEngine() is the GC355 vg_lite engine, drawing into the buffer we
// pass (m_image's bits).  Unresolved -> null -> plain QImage raster.
extern "C" __attribute__((weak))
QPaintDevice *qzephyr_make_vglite_paint_device(uchar *buffer, int w, int h, int strideBytes);

QPaintDevice *QZephyrBackingStore::paintDevice()
{
    if (qzephyr_make_vglite_paint_device && !m_image.isNull()) {
        if (!m_vgliteDevice)
            m_vgliteDevice = qzephyr_make_vglite_paint_device(
                m_image.bits(), m_image.width(), m_image.height(),
                int(m_image.bytesPerLine()));
        if (m_vgliteDevice)
            return m_vgliteDevice;
    }
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

    auto *pw = static_cast<QZephyrWindow *>(window()->handle());
    if (pw)
        pw->setBackingStoreImage(&m_image);

    // The vglite paint device wrapped the old buffer; drop it so paintDevice()
    // recreates it over the new m_image.
    delete m_vgliteDevice;
    m_vgliteDevice = nullptr;
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

// PXP hardware compositor (Stage 2, CONFIG_QT_PXP_COMPOSITE).  Routes a window
// to the PXP PS (background) or AS (overlay) layer by its StaysOnTopHint flag;
// weak/unresolved -> fall back to the plain whole-frame qzephyr_display_write.
extern "C" __attribute__((weak))
void qzephyr_display_present_layer(int wx, int wy, int w, int h,
                                  int pitch_bytes, int bytes_per_pixel,
                                  const void *buf, int isOverlay);

// Weak hooks for runtime liveness monitoring.  Strong defs (see
// qt5/zephyr-module/src/qzephyr_display_zephyr.cpp) report sys_heap
// + input-queue stats so the bs.flush heartbeat can show whether the
// firmware is leaking memory or backpressuring on inputs over hours
// of runtime -- key for diagnosing "works first few minutes then
// dies" failure modes.
extern "C" __attribute__((weak)) int qzephyr_heap_free_bytes();
extern "C" __attribute__((weak)) int qzephyr_heap_alloc_bytes();
extern "C" __attribute__((weak)) int qzephyr_input_queue_used();
// Touch liveness counters; strong defs in qzephyr_touch_zephyr.cpp.
extern "C" __attribute__((weak)) int qzephyr_touch_evt_count();
extern "C" __attribute__((weak)) int qzephyr_touch_press_count();
extern "C" __attribute__((weak)) int qzephyr_touch_release_count();
extern "C" __attribute__((weak)) int qzephyr_main_stack_free_bytes();
// Opt-in (Stage 2, CONFIG_QT_PXP_PARTIAL_FLUSH): when true, flush() pushes only
// the changed region (2-frame union) to display_write instead of the whole
// frame.  Unresolved/weak -> false -> whole-frame (the safe historical default).
extern "C" __attribute__((weak)) bool qzephyr_pxp_partial_flush_enabled();

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
    // PXP hardware compositor (weak; strong def under CONFIG_QT_PXP_COMPOSITE).
    // Routes this window to the PXP PS (background) or AS (overlay) layer by its
    // Qt::WindowStaysOnTopHint flag and lets the PXP blend + rotate.  Absent ->
    // plain whole-frame display_write below.
    if (qzephyr_display_present_layer) {
        const bool overlay = window && (window->flags() & Qt::WindowStaysOnTopHint);
        const QRect g = window ? window->geometry()
                               : QRect(0, 0, m_image.width(), m_image.height());
        qzephyr_display_present_layer(g.x(), g.y(),
                                      m_image.width(), m_image.height(),
                                      m_image.bytesPerLine(), m_image.depth() / 8,
                                      m_image.constBits(), overlay ? 1 : 0);
        // Keep the QtQuick render loop running.  The M7-only display_write path
        // BLOCKS on the eLCDIF frame-done (VSYNC) semaphore, which paces and
        // sustains the basic render loop.  This dual-core present_layer returns
        // instantly, so the loop renders one frame and stops (it only re-arms
        // its update timer when updatePending was set during sync).  Actively
        // request the next frame so continuous animations keep painting without
        // needing an input event to wake the loop.
        if (window)
            window->requestUpdate();
        return;
    }
    Q_UNUSED(region);
    if (qzephyr_display_write) {
        QZephyrScreen *scr = window && window->screen()
            ? static_cast<QZephyrScreen *>(window->screen()->handle())
            : nullptr;
        const bool compositing = scr && scr->hasVisiblePopups();
        // Whole-frame flush regardless of dirty region: the only known
        // production path is the RT1170 LCDIF + PXP pipeline, and that
        // pipeline requires whole-frame source writes when configured
        // for hardware rotation (CONFIG_MCUX_ELCDIF_PXP_ROTATE_*).
        // Passing a sub-rect corrupts the rotated framebuffer on every
        // call after the first.  Bandwidth-wise this is fine: a 1280x720
        // RGB16 frame is ~1.84 MB and the panel only refreshes at 60 Hz.

        // Frame timing instrumentation.  Three numbers per sample,
        // reported once per second:
        //   paint_ms = time from previous display_write return to this
        //              flush entry == Qt scene-graph + QPainter cost
        //   dma_ms   = display_write duration == PXP DMA wait
        //   frame_ms = total frame interval == paint + dma + other
        // FPS = 1000 / frame_ms.  Use this to decide whether to chase
        // paint (scene invalidation hotspots) or DMA (LCDIF tuning).
        static QElapsedTimer s_timer;
        static qint64 s_lastDmaEndNs = 0;
        static qint64 s_lastFlushNs = 0;
        static int s_samples = 0;
        static qint64 s_sumPaintNs = 0;
        static qint64 s_sumDmaNs = 0;
        static qint64 s_sumFrameNs = 0;
        static qint64 s_lastReportNs = 0;
        static qint64 s_sumDirtyPx = 0;
        static qint64 s_sumTotalPx = 0;
        if (!s_timer.isValid())
            s_timer.start();
        const qint64 t_entry = s_timer.nsecsElapsed();

        const int bpp = m_image.depth() / 8;
        const int pitch = m_image.bytesPerLine();
        // Partial flush (opt-in via the weak qzephyr_pxp_partial_flush_enabled()
        // hook, set by CONFIG_QT_PXP_PARTIAL_FLUSH): push only the region that
        // changed over the last TWO frames.  The union covers the alternate
        // double-buffer (a single-frame sub-rect leaves the other buffer one
        // frame stale -> trails, which is why the historical sub-rect attempt
        // corrupted; static pixels were written to both buffers when they first
        // appeared).  Falls back to whole-frame when disabled, when the region
        // is empty/unknown, or when the union covers most of the frame
        // (scattered dirty -> bounding box ~ full frame -> no win).
        bool wholeFrame = true;
        if (!compositing
            && qzephyr_pxp_partial_flush_enabled && qzephyr_pxp_partial_flush_enabled()
            && !region.isEmpty()) {
            static QRegion s_prevFlush;
            const QRegion paintRegion = region | s_prevFlush;
            s_prevFlush = region;
            const QRect full(0, 0, m_image.width(), m_image.height());
            const QRect br = paintRegion.boundingRect().intersected(full);
            const qint64 brPx = qint64(br.width()) * br.height();
            const qint64 totPx = qint64(full.width()) * full.height();
            if (!br.isEmpty() && brPx * 4 < totPx * 3) {   // saves >=25%
                const uchar *sub = m_image.constBits()
                        + qint64(br.y()) * pitch + qint64(br.x()) * bpp;
                qzephyr_display_write(br.x(), br.y(), br.width(), br.height(),
                                      pitch, bpp, sub);
                wholeFrame = false;
            }
        }
        if (wholeFrame) {
            if (compositing) {
                const QImage &comp = scr->composite();
                qzephyr_display_write(0, 0, comp.width(), comp.height(),
                                      comp.bytesPerLine(), comp.depth() / 8,
                                      comp.constBits());
            } else {
                qzephyr_display_write(0, 0, m_image.width(), m_image.height(),
                                      pitch, bpp, m_image.constBits());
            }
        }

        const qint64 t_done = s_timer.nsecsElapsed();
        if (s_lastDmaEndNs != 0) {
            s_sumPaintNs += (t_entry - s_lastDmaEndNs);
            s_sumDmaNs   += (t_done  - t_entry);
            s_sumFrameNs += (t_entry - s_lastFlushNs);
            const QRect br = region.boundingRect();
            const qint64 dirtyPx = qint64(br.width()) * br.height();
            const qint64 totalPx = qint64(m_image.width()) * m_image.height();
            s_sumDirtyPx += dirtyPx;
            s_sumTotalPx += totalPx;
            ++s_samples;
            // Report every 5 seconds.  A 1-line heartbeat is fine
            // (polling UART blocks main for ~9 ms once per 5 s = 0.2%
            // of the cycle, negligible vs the 23 ms PXP wait per frame)
            // and we need it to know whether the firmware is alive
            // during touch experiments where the user isn't tapping.
            if (t_done - s_lastReportNs > 5'000'000'000LL) {
                const qint64 avgPaintUs = (s_sumPaintNs / s_samples) / 1000;
                const qint64 avgDmaUs   = (s_sumDmaNs   / s_samples) / 1000;
                const qint64 avgFrameUs = (s_sumFrameNs / s_samples) / 1000;
                const double dirtyPct = s_sumTotalPx > 0
                    ? 100.0 * double(s_sumDirtyPx) / double(s_sumTotalPx) : 0.0;
                // heap stats currently unimplemented (linker conflict with
                // libc malloc + nano-malloc); dropped from heartbeat.
                const int evtN  = qzephyr_touch_evt_count     ? qzephyr_touch_evt_count()     : -1;
                const int prsN  = qzephyr_touch_press_count   ? qzephyr_touch_press_count()   : -1;
                const int relN  = qzephyr_touch_release_count ? qzephyr_touch_release_count() : -1;
                const int stkFree = qzephyr_main_stack_free_bytes ? qzephyr_main_stack_free_bytes() : -1;
                qDebug("[bs.flush] paint=%lldus dma=%lldus frame=%lldus fps=%.1f dirty=%.1f%% stk_free=%dK touch evt=%d prs=%d rel=%d",
                       (long long)avgPaintUs,
                       (long long)avgDmaUs, (long long)avgFrameUs,
                       avgFrameUs > 0 ? 1.0e6 / double(avgFrameUs) : 0.0,
                       dirtyPct,
                       stkFree / 1024,
                       evtN, prsN, relN);
                s_lastReportNs = t_done;
                s_samples = 0;
                s_sumPaintNs = s_sumDmaNs = s_sumFrameNs = 0;
                s_sumDirtyPx = s_sumTotalPx = 0;
            }
        }
        s_lastDmaEndNs = t_done;
        s_lastFlushNs  = t_entry;
    } else {
        qDebug() << "QZephyrBackingStore::flush (no qzephyr_display_write hook) region:"
                 << region.boundingRect();
    }
#endif
}

QT_END_NAMESPACE
