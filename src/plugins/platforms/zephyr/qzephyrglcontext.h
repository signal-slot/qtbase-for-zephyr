// Copyright (C) 2026 Signal Slot Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QZEPHYRGLCONTEXT_H
#define QZEPHYRGLCONTEXT_H

#include <QtGui/private/qeglplatformcontext_p.h>

QT_BEGIN_NAMESPACE

// OpenGL ES 2.0 context over the EGL an on-target GL library provides
// (YakoGL on the TI AM62P: no window system, a "native window" is a
// scan-out pixel buffer the application owns).  Qt's generic EGL
// context does everything except two things this class adds:
//
//  - a window's EGLSurface is one of several single-buffered surfaces,
//    one per scan-out buffer (QZephyrWindow owns them), and the buffer
//    the next frame renders into changes after every swap;
//  - swapBuffers() drives that rotation and the display flip through
//    the Stage 2 hooks (see qzephyrwindow.cpp).
class QZephyrGLContext : public QEGLPlatformContext
{
public:
    QZephyrGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display);

    void swapBuffers(QPlatformSurface *surface) override;

protected:
    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) override;
};

QT_END_NAMESPACE

#endif // QZEPHYRGLCONTEXT_H
