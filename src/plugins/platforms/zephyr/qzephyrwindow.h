// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QZEPHYRWINDOW_H
#define QZEPHYRWINDOW_H

#include <qpa/qplatformwindow.h>
#include <QElapsedTimer>
#ifdef QZEPHYR_WITH_EGL
#include <QtCore/qvarlengtharray.h>
#include <QtGui/private/qt_egl_p.h>   // EGL/egl.h + the GL library's eglplatform.h (struct gles_native_window)
#endif

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

#ifdef QZEPHYR_WITH_EGL
    // OpenGL ES swap chain over the board's scan-out buffers.  Each
    // buffer is one single-buffered EGL window surface; the window
    // renders into them in turn.  All three are called by
    // QZephyrGLContext only.
    EGLSurface eglSurface(EGLDisplay display, EGLConfig config);   // the surface the next frame renders into
    void waitScanoutReleased();                                    // block until the display has left that buffer
    void frameSwapped(EGLDisplay display);                         // after eglSwapBuffers: present, rotate
#endif

private:
    QRect m_geometry;
    bool m_visible;
    WId m_winId;

#ifdef QZEPHYR_WITH_EGL
    struct ScanoutBuffer {
        gles_native_window win;
        EGLSurface surface = EGL_NO_SURFACE;
        quint32 releasedAt = 0;   // vsync count after which the display no longer reads it (0: never shown)
    };
    bool createSurfaces(EGLDisplay display, EGLConfig config);
    void destroySurfaces();
    void present(int index);
    void presentPendingWhenDone();

    QVarLengthArray<ScanoutBuffer, 3> m_buffers;
    EGLDisplay m_display = EGL_NO_DISPLAY;
    EGLSync m_pendingSync = EGL_NO_SYNC;   // completion of the frame in m_pending
    int m_current = 0;     // buffer the next frame renders into
    int m_pending = -1;    // buffer whose frame is submitted but not yet shown
    // QZEPHYR_FRAME_LOG: where a frame's wall clock went
    QElapsedTimer m_swapClock;
    qint64 m_lastBufferWaitMs = 0;
    qint64 m_lastSwapMs = 0;
    int m_onScreen = -1;   // buffer the display scans out
    bool m_pollScheduled = false;
#endif
};

QT_END_NAMESPACE

#endif // QZEPHYRWINDOW_H
