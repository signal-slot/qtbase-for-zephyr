// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Minimal SDL2 bridge for Zephyr native_sim_64 backend
#pragma once

#ifdef QZEPHYR_WITH_SDL

#include <QtGui/QImage>
#include <QtGui/QRegion>

void qzephyr_sdl_init(int w, int h, bool rgb888);
void qzephyr_sdl_present(const QImage &img, const QRegion &region);
void qzephyr_sdl_pollevents();

#endif // QZEPHYR_WITH_SDL

