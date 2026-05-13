// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qzephyrintegration.h"
#include "qzephyrscreen.h"
#include "qzephyrwindow.h"
#include "qzephyrbackingstore.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
// QFreeTypeFontDatabase is the lightweight base of QGenericUnixFontDatabase
// (which adds fontconfig).  On Zephyr we have no fontconfig and no system
// font directories, so the bare FreeType database is the right choice --
// apps load .ttf files via QFontDatabase::addApplicationFont.
#include <QtGui/private/qfreetypefontdatabase_p.h>
#ifdef QZEPHYR_WITH_SDL
// Host x11-sim build: the system has /usr/share/fonts populated and
// fontconfig running, so use the fontconfig-aware font DB to avoid
// rendering everything as tofu.
#include <QtGui/private/qfontconfigdatabase_p.h>
#endif
#ifdef QZEPHYR_WITH_SDL
#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>
// Host x11sim only -- need a real OS event dispatcher when the Zephyr-
// runtime hook isn't linked.  This header is part of the Qt private API
// but always shipped in the qtbase install, so it's safe to use here.
#include <QtGui/private/qgenericunixeventdispatcher_p.h>

// Minimal SDL event pump hooked into Qt timers
extern void qzephyr_sdl_init(int w, int h, bool rgb888);
extern void qzephyr_sdl_pollevents();
#endif

QT_BEGIN_NAMESPACE

QZephyrIntegration::QZephyrIntegration()
    : m_primaryScreen(nullptr)
    , m_fontDb(nullptr)
{
}

QZephyrIntegration::~QZephyrIntegration()
{
    delete m_fontDb;
    delete m_primaryScreen;
}

void QZephyrIntegration::initialize()
{
    // Create a default screen
    m_primaryScreen = new QZephyrScreen();
    QWindowSystemInterface::handleScreenAdded(m_primaryScreen);

#ifdef QZEPHYR_WITH_SDL
    m_fontDb = new QFontconfigDatabase();
#else
    m_fontDb = new QFreeTypeFontDatabase();
#endif

#ifdef QZEPHYR_WITH_SDL
    const bool rgb888 = (m_primaryScreen->format() == QImage::Format_RGB888);
    qzephyr_sdl_init(m_primaryScreen->geometry().width(), m_primaryScreen->geometry().height(), rgb888);

    // Poll SDL events regularly to generate Qt events
    static QPointer<QObject> sdlPump;
    if (!sdlPump) {
        QObject *pump = new QObject;
        QTimer *t = new QTimer(pump);
        QObject::connect(t, &QTimer::timeout, pump, [](){ qzephyr_sdl_pollevents(); });
        t->start(10);
        sdlPump = pump;
    }
#endif
}

bool QZephyrIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
        return true;
    case MultipleWindows:
        return false;  // Zephyr typically runs single app
    case NonFullScreenWindows:
        return false;
    case OpenGL:
        return false;  // No OpenGL support initially
    case ThreadedOpenGL:
        return false;
    case SharedGraphicsCache:
        return false;
    case BufferQueueingOpenGL:
        return false;
    case PaintEvents:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QZephyrIntegration::createPlatformWindow(QWindow *window) const
{
    Q_UNUSED(window);
    return new QZephyrWindow(window);
}

QPlatformBackingStore *QZephyrIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QZephyrBackingStore(window);
}

// Weak hook supplied by the Stage 2 Zephyr application -- see
// examples/zephyr-rt1170-gui/src/qzephyr_display_zephyr.cpp.  Returns a
// freshly-allocated QEventDispatcherZephyr.  Stage 1 cannot include
// <zephyr/kernel.h> so we cannot `new QEventDispatcherZephyr` directly
// here; the hook lives in the Zephyr-aware TU instead.
extern "C" __attribute__((weak))
QAbstractEventDispatcher *qzephyr_make_event_dispatcher();

QAbstractEventDispatcher *QZephyrIntegration::createEventDispatcher() const
{
    // QGuiApplication ctor requires a non-null dispatcher from the
    // integration (unlike QCoreApplication, which falls back to
    // QThreadPrivate::createEventDispatcher).
    if (qzephyr_make_event_dispatcher)
        return qzephyr_make_event_dispatcher();
#ifdef QZEPHYR_WITH_SDL
    // Host x11-sim path: there is no Zephyr runtime so the weak hook is
    // absent.  Fall back to Qt's generic Unix dispatcher, the same one
    // every other UNIX QPA (eglfs, linuxfb, xcb) uses.
    return createUnixEventDispatcher();
#else
    qDebug("QZephyrIntegration: no qzephyr_make_event_dispatcher hook -- QGuiApplication will fault");
    return nullptr;
#endif
}

QPlatformFontDatabase *QZephyrIntegration::fontDatabase() const
{
    return m_fontDb;
}

QPlatformNativeInterface *QZephyrIntegration::nativeInterface() const
{
    return const_cast<QZephyrIntegration *>(this);
}

QT_END_NAMESPACE
