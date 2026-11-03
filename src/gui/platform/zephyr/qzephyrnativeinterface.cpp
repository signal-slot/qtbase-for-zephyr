// Copyright (C) 2026 Signal Slot Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// The EGL native-interface definitions Qt only ships in
// platform/unix/qunixnativeinterface.cpp (built on UNIX).  Zephyr is not
// UNIX to CMake, but its qzephyr platform plugin derives its context from
// QEGLPlatformContext, which implements QNativeInterface::QEGLContext, so
// the interface's out-of-line members must exist.

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface::Private;

QT_DEFINE_NATIVE_INTERFACE(QEGLContext);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QEGLIntegration);

QOpenGLContext *QNativeInterface::QEGLContext::fromNative(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QEGLIntegration::createOpenGLContext>(context, display, shareContext);
}

QT_END_NAMESPACE
