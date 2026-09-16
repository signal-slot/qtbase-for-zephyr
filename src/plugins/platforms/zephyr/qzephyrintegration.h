// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QZEPHYRINTEGRATION_H
#define QZEPHYRINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformscreen.h>
#ifdef QZEPHYR_WITH_EGL
#include <QtGui/private/qt_egl_p.h>
#endif

QT_BEGIN_NAMESPACE

class QZephyrScreen;

// Multi-inherit QPlatformNativeInterface so that QGuiApplication::platformNativeInterface()
// returns a non-null pointer. QtGui code (e.g. QFontconfigDatabase::setupFontEngine) calls
// QGuiApplication::platformNativeInterface()->nativeResourceForScreen(...) unconditionally;
// without an interface object that call dereferences nullptr and SIGSEGVs. The base
// QPlatformNativeInterface returns nullptr from its virtuals by default, which the callers
// handle safely.
class QZephyrIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    QZephyrIntegration();
    ~QZephyrIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;
    QPlatformFontDatabase *fontDatabase() const override;
    QPlatformNativeInterface *nativeInterface() const override;

    void initialize() override;

#ifdef QZEPHYR_WITH_EGL
    // OpenGL ES 2.0 over the on-target EGL (YakoGL).  Present only when
    // Stage 2 links the GL display hooks (qzephyr_gl_*); hasCapability
    // reports OpenGL accordingly.
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;
    void *nativeResourceForIntegration(const QByteArray &resource) override;
    EGLDisplay eglDisplay() const { return m_eglDisplay; }
#endif

private:
    QZephyrScreen *m_primaryScreen;
    QPlatformFontDatabase *m_fontDb;
#ifdef QZEPHYR_WITH_EGL
    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
#endif
};

QT_END_NAMESPACE

#endif // QZEPHYRINTEGRATION_H