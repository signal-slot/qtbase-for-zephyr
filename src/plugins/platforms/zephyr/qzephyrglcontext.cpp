// Copyright (C) 2026 Signal Slot Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qzephyrglcontext.h"
#include "qzephyrwindow.h"

#include <QtGui/private/qeglpbuffer_p.h>
#include <QtGui/qsurface.h>

QT_BEGIN_NAMESPACE

QZephyrGLContext::QZephyrGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share,
                                   EGLDisplay display)
    // NoSurfaceless: the on-target EGL has no EGL_KHR_surfaceless_context;
    // a context made current without a window gets a pbuffer instead.
    : QEGLPlatformContext(format, share, display, nullptr, QEGLPlatformContext::NoSurfaceless)
{
}

EGLSurface QZephyrGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window)
        return static_cast<QZephyrWindow *>(surface)->eglSurface(eglDisplay(), eglConfig());
    return static_cast<QEGLPbuffer *>(surface)->pbuffer();
}

void QZephyrGLContext::swapBuffers(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() != QSurface::Window) {
        QEGLPlatformContext::swapBuffers(surface);
        return;
    }

    QZephyrWindow *w = static_cast<QZephyrWindow *>(surface);
    // The buffer this frame renders into may still be on screen: the GPU
    // must not start writing it before the display has moved off it.
    w->waitScanoutReleased();
    // Submits this frame; returns once the previous one has completed.
    QEGLPlatformContext::swapBuffers(surface);
    // Show the completed frame, rotate to the next scan-out buffer.
    w->frameSwapped(eglDisplay());
    // Point the context at the buffer the next frame renders into now.
    // QRhi's next beginFrame() makes the same (window, context) pair
    // current again and may treat that as a no-op, so do not rely on it.
    EGLSurface next = w->eglSurface(eglDisplay(), eglConfig());
    eglMakeCurrent(eglDisplay(), next, next, eglContext());
}

QT_END_NAMESPACE
