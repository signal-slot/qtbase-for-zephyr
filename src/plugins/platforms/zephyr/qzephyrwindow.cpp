// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qzephyrwindow.h"
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformscreen.h>
#include <QtGui/qwindow.h>
#include <QtCore/qdebug.h>
#ifdef QZEPHYR_WITH_EGL
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qtimer.h>
#include <GLES2/gl2.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef QZEPHYR_WITH_EGL
// Weak hooks supplied by the Stage 2 Zephyr application (the display
// glue that can include the board's display driver headers; on the
// AM62P: zephyr-module/src/qzephyr_display_dss.cpp).  Stage 1 cannot
// include <zephyr/...>, so the plugin only calls through these.  When
// none is linked the platform reports no OpenGL capability.
extern "C" __attribute__((weak))
int qzephyr_gl_buffer_count(void);
extern "C" __attribute__((weak))
bool qzephyr_gl_native_window(int index, struct gles_native_window *out);
extern "C" __attribute__((weak))
void qzephyr_gl_present(int index);
extern "C" __attribute__((weak))
unsigned int qzephyr_gl_vsync_count(void);
extern "C" __attribute__((weak))
void qzephyr_gl_wait_vsync(unsigned int count);   // block until the vsync counter reaches `count`
extern "C" __attribute__((weak))
void qzephyr_gl_debug_set(int on);                // GL library per-draw dump (debug builds only)

bool qzephyr_gl_available()
{
    return qzephyr_gl_buffer_count && qzephyr_gl_native_window && qzephyr_gl_present;
}
#endif

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
#ifdef QZEPHYR_WITH_EGL
    destroySurfaces();
#endif
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

#ifdef QZEPHYR_WITH_EGL

// ---- OpenGL ES swap chain ---------------------------------------------
//
// The on-target EGL (YakoGL) has single-buffered window surfaces over
// caller-owned pixel buffers, and eglSwapBuffers() submits the frame
// and returns once the *previous* frame has completed (the CPU records
// frame N+1 while the GPU renders frame N).  Double/triple buffering is
// therefore the client's job, and this is the same scheme the
// gl_slint sample of the GPU driver repository uses:
//
//   swap(cur):  wait until the display has left `cur`
//               eglSwapBuffers(cur)            frame N submitted, N-1 done
//               present(pending = N-1)         on screen at the next vsync
//               pending = cur; cur = next
//
// A UI that stops rendering (Qt Quick after the last property change)
// would leave its final frame pending forever, so a fence sync taken
// right after the swap is polled from the event loop and the pending
// frame is presented as soon as the GPU has finished it.  With
// continuous animation the next swap presents it first and the poll
// finds nothing to do, so pipelining is kept.

bool QZephyrWindow::createSurfaces(EGLDisplay display, EGLConfig config)
{
    const int count = qMax(1, qzephyr_gl_buffer_count());
    m_display = display;
    m_buffers.clear();
    for (int i = 0; i < count; ++i) {
        ScanoutBuffer b;
        memset(&b.win, 0, sizeof(b.win));
        if (!qzephyr_gl_native_window(i, &b.win)) {
            qWarning("QZephyrWindow: scan-out buffer %d unavailable", i);
            break;
        }
        b.surface = eglCreateWindowSurface(display, config, &b.win, nullptr);
        if (b.surface == EGL_NO_SURFACE) {
            qWarning("QZephyrWindow: eglCreateWindowSurface(buffer %d) failed: 0x%x", i, eglGetError());
            break;
        }
        m_buffers.append(b);
    }
    if (m_buffers.isEmpty())
        return false;
    // The GL viewport is the buffer, whatever QWindow asked for.
    if (m_geometry.size() != QSize(int(m_buffers[0].win.width), int(m_buffers[0].win.height))) {
        m_geometry.setSize(QSize(int(m_buffers[0].win.width), int(m_buffers[0].win.height)));
        QWindowSystemInterface::handleGeometryChange(window(), m_geometry);
    }
    m_current = 0;
    m_pending = -1;
    m_onScreen = -1;
    if (qEnvironmentVariableIsSet("QZEPHYR_FRAME_LOG") && qzephyr_gl_debug_set)
        qzephyr_gl_debug_set(1);   // dump the first frames' draws; frameSwapped() turns it off
    return true;
}

void QZephyrWindow::destroySurfaces()
{
    if (m_pendingSync != EGL_NO_SYNC) {
        eglDestroySync(m_display, m_pendingSync);
        m_pendingSync = EGL_NO_SYNC;
    }
    for (ScanoutBuffer &b : m_buffers) {
        if (b.surface != EGL_NO_SURFACE)
            eglDestroySurface(m_display, b.surface);
    }
    m_buffers.clear();
}

EGLSurface QZephyrWindow::eglSurface(EGLDisplay display, EGLConfig config)
{
    if (m_buffers.isEmpty() && !createSurfaces(display, config))
        return EGL_NO_SURFACE;
    return m_buffers[m_current].surface;
}

void QZephyrWindow::waitScanoutReleased()
{
    if (m_buffers.size() < 2)
        return;
    const ScanoutBuffer &b = m_buffers[m_current];
    QElapsedTimer clock;
    clock.start();
    if (b.releasedAt != 0 && qzephyr_gl_vsync_count && qzephyr_gl_wait_vsync
        && int(qzephyr_gl_vsync_count() - b.releasedAt) < 0)
        qzephyr_gl_wait_vsync(b.releasedAt);
    m_lastBufferWaitMs = clock.elapsed();
    m_swapClock.start();   /* the GL work of this frame starts here */
}

// QZEPHYR_FRAME_LOG=1 (set by the Stage 2 main wrapper under
// CONFIG_QT_DEBUG_LOG): one console line per 60 frames and on every GL
// error, the only per-frame visibility a board without a shell has.
static bool frameLogEnabled()
{
    static const bool on = qEnvironmentVariableIsSet("QZEPHYR_FRAME_LOG");
    return on;
}

void QZephyrWindow::present(int index)
{
    if (frameLogEnabled()) {
        static unsigned presents = 0;
        if (++presents % 60 == 1)
            qDebug("QZephyrWindow: present #%u buffer %d (vsync %u)", presents, index,
                   qzephyr_gl_vsync_count ? qzephyr_gl_vsync_count() : 0u);
    }
    qzephyr_gl_present(index);
    if (m_onScreen >= 0 && m_onScreen != index) {
        // The flip takes effect at the next vsync; the old buffer is
        // being scanned out until then.
        m_buffers[m_onScreen].releasedAt = (qzephyr_gl_vsync_count ? qzephyr_gl_vsync_count() : 0) + 1;
    }
    m_onScreen = index;
}

void QZephyrWindow::frameSwapped(EGLDisplay display)
{
    if (m_buffers.isEmpty())
        return;
    const int cur = m_current;
    m_lastSwapMs = m_swapClock.isValid() ? m_swapClock.elapsed() : 0;
    if (frameLogEnabled()) {
        static unsigned frames = 0;
        static QElapsedTimer clock;
        if (frames == 0)
            clock.start();
        ++frames;
        const GLenum err = glGetError();
        if (err != GL_NO_ERROR || frames % 60 == 1)
            qDebug("QZephyrWindow: frame %u into buffer %d after %lld ms (glGetError 0x%x, pending %d, on screen %d)",
                   frames, cur, (long long)clock.elapsed(), err, m_pending, m_onScreen);
        if (frames == 1 || frames == 2 || frames == 120) {
            // Ground truth of what the GPU wrote: a few pixels of the frame
            // just submitted (this surface is still the draw surface).
            static const int pts[][2] = { {8, 8}, {8, 591}, {200, 300}, {512, 80}, {512, 540}, {900, 300} };
            for (const auto &pt : pts) {
                unsigned char px[4] = { 0, 0, 0, 0 };
                glReadPixels(pt[0], int(m_buffers[cur].win.height) - 1 - pt[1], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
                qDebug("QZephyrWindow: frame %u pixel (%d,%d) = %u %u %u %u", frames, pt[0], pt[1], px[0], px[1], px[2], px[3]);
            }
            if (frames == 2 && qzephyr_gl_debug_set)
                qzephyr_gl_debug_set(0);
        }
    }
    if (m_pendingSync != EGL_NO_SYNC) {
        eglDestroySync(display, m_pendingSync);
        m_pendingSync = EGL_NO_SYNC;
    }
    if (m_buffers.size() == 1) {
        // Single buffer: nothing to pipeline, show it when it is done.
        m_pending = cur;
    } else {
        // eglSwapBuffers returned when the previous frame completed:
        // that one goes on screen, the one just submitted is pending.
        if (m_pending >= 0)
            present(m_pending);
        m_pending = cur;
        m_current = (cur + 1) % m_buffers.size();
    }
    m_pendingSync = eglCreateSync(display, EGL_SYNC_FENCE, nullptr);
    presentPendingWhenDone();

    // Swap interval 1 (the default): pace the render loop to the panel.
    // Rendering runs far ahead of the 58 Hz scan-out otherwise (frames
    // nobody sees, at full CPU/GPU load), and a flip issued more than once
    // per vsync would overrun the previous one.  QZEPHYR_NO_VSYNC=1
    // removes the wait for throughput measurements.
    static const bool noVsync = qEnvironmentVariableIsSet("QZEPHYR_NO_VSYNC");
    QElapsedTimer vsyncClock;
    if (frameLogEnabled())
        vsyncClock.start();
    if (!noVsync && m_buffers.size() > 1 && window()->requestedFormat().swapInterval() != 0
        && qzephyr_gl_vsync_count && qzephyr_gl_wait_vsync)
        qzephyr_gl_wait_vsync(qzephyr_gl_vsync_count() + 1);
    if (frameLogEnabled()) {
        // Where a frame's wall clock goes: the wait for the buffer the display
        // still holds, the GL work up to the swap, and the wait for the panel.
        static unsigned n = 0;
        if (++n % 60 == 1)
            qDebug("QZephyrWindow: frame %u waits: buffer %lld ms, swap %lld ms, vsync %lld ms",
                   n, (long long)m_lastBufferWaitMs, (long long)m_lastSwapMs,
                   (long long)vsyncClock.elapsed());
    }
}

void QZephyrWindow::presentPendingWhenDone()
{
    if (m_pending < 0 || m_pollScheduled)
        return;
    m_pollScheduled = true;
    // Poll from the event loop rather than blocking here, so a
    // continuously animating scene keeps the CPU/GPU overlap.
    QTimer::singleShot(2, window(), [this]() {
        m_pollScheduled = false;
        if (m_pending < 0)
            return;
        const bool done = m_pendingSync == EGL_NO_SYNC
                || eglClientWaitSync(m_display, m_pendingSync, 0, 0) == EGL_CONDITION_SATISFIED;
        if (!done) {
            presentPendingWhenDone();
            return;
        }
        present(m_pending);
        m_pending = -1;
        if (m_pendingSync != EGL_NO_SYNC) {
            eglDestroySync(m_display, m_pendingSync);
            m_pendingSync = EGL_NO_SYNC;
        }
    });
}

#endif // QZEPHYR_WITH_EGL

QT_END_NAMESPACE
