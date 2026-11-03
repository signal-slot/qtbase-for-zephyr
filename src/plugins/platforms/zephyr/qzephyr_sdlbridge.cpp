// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qzephyr_sdlbridge.h"

#ifdef QZEPHYR_WITH_SDL

#include <SDL.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/QGuiApplication>
#include <QtCore/QEvent>
#include <QtCore/QDebug>

static SDL_Window *s_window = nullptr;
static SDL_Renderer *s_renderer = nullptr;
static SDL_Texture *s_texture = nullptr;
static int s_w = 0, s_h = 0;
static bool s_rgb888 = false;

void qzephyr_sdl_init(int w, int h, bool rgb888)
{
    if (s_window)
        return;

    s_w = w; s_h = h; s_rgb888 = rgb888;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        qWarning() << "SDL_Init failed:" << SDL_GetError();
        return;
    }
    qInfo() << "qzephyr: SDL video driver" << SDL_GetCurrentVideoDriver()
            << "size" << w << "x" << h << "rgb888=" << rgb888;
    s_window = SDL_CreateWindow("Qt on Zephyr (native_sim)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);
    if (!s_window) {
        qWarning() << "SDL_CreateWindow failed:" << SDL_GetError();
        return;
    }
    s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!s_renderer)
        s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_SOFTWARE);
    if (!s_renderer) {
        qWarning() << "SDL_CreateRenderer failed:" << SDL_GetError();
        return;
    }
    Uint32 fmt = rgb888 ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGB565;
    s_texture = SDL_CreateTexture(s_renderer, fmt, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!s_texture) {
        qWarning() << "SDL_CreateTexture failed:" << SDL_GetError();
        return;
    }
}

void qzephyr_sdl_present(const QImage &img, const QRegion &region)
{
    if (!s_renderer || !s_texture)
        return;

    // Upload full frame for simplicity; region ignored for now
    void *pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(s_texture, nullptr, &pixels, &pitch) == 0) {
        const uchar *src = img.bits();
        const int bpp = s_rgb888 ? 3 : 2;
        for (int y = 0; y < s_h && y < img.height(); ++y) {
            memcpy((uchar*)pixels + y * pitch, src + y * img.bytesPerLine(), qMin(s_w, img.width()) * bpp);
        }
        SDL_UnlockTexture(s_texture);
        SDL_RenderClear(s_renderer);
        SDL_RenderCopy(s_renderer, s_texture, nullptr, nullptr);
        SDL_RenderPresent(s_renderer);
    }
}

void qzephyr_sdl_pollevents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            QWindowSystemInterface::handleApplicationTermination();
            break;
        case SDL_MOUSEMOTION: {
            const QPointF pos(e.motion.x, e.motion.y);
            QWindowSystemInterface::handleMouseEvent(nullptr, pos, pos,
                QGuiApplication::mouseButtons(), Qt::NoButton, QEvent::MouseMove,
                Qt::NoModifier);
            break; }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            Qt::MouseButton b = Qt::NoButton;
            if (e.button.button == SDL_BUTTON_LEFT) b = Qt::LeftButton;
            else if (e.button.button == SDL_BUTTON_RIGHT) b = Qt::RightButton;
            else if (e.button.button == SDL_BUTTON_MIDDLE) b = Qt::MiddleButton;
            const QPointF pos(e.button.x, e.button.y);
            Qt::MouseButtons buttons = QGuiApplication::mouseButtons();
            const bool down = (e.type == SDL_MOUSEBUTTONDOWN);
            if (down) buttons |= b; else buttons &= ~b;
            QWindowSystemInterface::handleMouseEvent(nullptr, pos, pos, buttons, b,
                down ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease,
                Qt::NoModifier);
            break; }
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            const bool down = (e.type == SDL_KEYDOWN);
            QWindowSystemInterface::handleKeyEvent(nullptr, down ? QEvent::KeyPress : QEvent::KeyRelease, e.key.keysym.sym, Qt::NoModifier);
            break; }
        case SDL_WINDOWEVENT: {
            // X11 window lifecycle: when the user iconifies/un-iconifies
            // the SDL window or another window occludes it, Qt's scene
            // graph stops painting until we post a fresh expose event.
            // Re-expose on SHOWN / EXPOSED / RESTORED / RESIZED /
            // FOCUS_GAINED; mark the window non-exposed on
            // HIDDEN / MINIMIZED so QPainter-backed widgets don't keep
            // producing frames noone sees.
            QWindow *target = nullptr;
            const auto windows = QGuiApplication::topLevelWindows();
            if (!windows.isEmpty())
                target = windows.constFirst();
            if (!target)
                break;
            switch (e.window.event) {
            case SDL_WINDOWEVENT_SHOWN:
            case SDL_WINDOWEVENT_EXPOSED:
            case SDL_WINDOWEVENT_RESTORED:
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                QWindowSystemInterface::handleExposeEvent(target, QRect(QPoint(0, 0), target->size()));
                break;
            case SDL_WINDOWEVENT_HIDDEN:
            case SDL_WINDOWEVENT_MINIMIZED:
                QWindowSystemInterface::handleExposeEvent(target, QRegion());
                break;
            default:
                break;
            }
            break; }
        default:
            break;
        }
    }
}

#endif // QZEPHYR_WITH_SDL

