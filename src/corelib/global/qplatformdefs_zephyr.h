/****************************************************************************
**
** Copyright (C) 2025 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPLATFORMDEFS_ZEPHYR_H
#define QPLATFORMDEFS_ZEPHYR_H

// Get Zephyr OS definitions
#include <zephyr/kernel.h>
#include <zephyr/types.h>

// Zephyr-specific type definitions
#define QT_FOPEN                k_malloc
#define QT_FCLOSE               k_free
#define QT_FSEEK                ::fseek
#define QT_FTELL                ::ftell
#define QT_FGETPOS              ::fgetpos
#define QT_FSETPOS              ::fsetpos
#define QT_MMAP                 ::mmap
#define QT_FPOS_T               fpos_t
#define QT_OFF_T                off_t

// Memory allocation using Zephyr's k_malloc/k_free
#define QT_MALLOC               k_malloc
#define QT_FREE                 k_free
#define QT_REALLOC              k_realloc
#define QT_CALLOC               k_calloc

// Thread-related definitions for Zephyr
#define QT_THREAD_SUPPORT

// Socket-related definitions (not supported on Zephyr)
#define QT_NO_SOCKET_H

// Signal-related definitions (not supported on Zephyr)
#define QT_NO_SIGNALING_FUNCTION

// Process-related definitions (not supported on Zephyr)
#define QT_NO_PROCESS

// Library loading (not supported on Zephyr)
#define QT_NO_LIBRARY

// Shared memory and system semaphore (not supported on Zephyr)
#define QT_NO_SHAREDMEMORY
#define QT_NO_SYSTEMSEMAPHORE

// File system features
#define QT_NO_FSFILEENGINE
#define QT_NO_FILESYSTEMMODEL
#define QT_NO_FILESYSTEMWATCHER

// Networking (not supported in this minimal port)
#define QT_NO_NETWORK

// D-Bus (not supported on Zephyr)
#define QT_NO_DBUS

// Standard integer types
typedef int8_t qint8;
typedef uint8_t quint8;
typedef int16_t qint16;
typedef uint16_t quint16;
typedef int32_t qint32;
typedef uint32_t quint32;
typedef int64_t qint64;
typedef uint64_t quint64;

typedef qint64 qlonglong;
typedef quint64 qulonglong;

// Pointer size
#if defined(__ARM_32BIT_STATE) || defined(__arm__)
typedef quint32 quintptr;
typedef qint32 qptrdiff;
#else
typedef quint64 quintptr;
typedef qint64 qptrdiff;
#endif

// Basic POSIX-like definitions for compatibility
#define QT_STAT                 ::stat
#define QT_LSTAT                ::lstat
#define QT_STAT_REG             S_IFREG
#define QT_STAT_DIR             S_IFDIR
#define QT_STAT_LNK             S_IFLNK
#define QT_ACCESS               ::access
#define QT_OPEN                 ::open
#define QT_CLOSE                ::close
#define QT_READ                 ::read
#define QT_WRITE                ::write
#define QT_LSEEK                ::lseek
#define QT_TRUNCATE             ::truncate
#define QT_FTRUNCATE            ::ftruncate

// Math functions
#define QT_ISNAN(x)             isnan(x)
#define QT_ISINF(x)             isinf(x)
#define QT_ISFINITE(x)          isfinite(x)

// String functions
#define QT_STRNICMP             strncasecmp
#define QT_STRICMP              strcasecmp

// Time functions
#define QT_GETTIMEOFDAY         ::gettimeofday

// Directory functions (limited support)
#define QT_OPENDIR              ::opendir
#define QT_CLOSEDIR             ::closedir
#define QT_READDIR              ::readdir

// Disable features not available on Zephyr
#define QT_NO_CONCURRENT
#define QT_NO_SQL
#define QT_NO_XMLPATTERNS
#define QT_NO_DOM
#define QT_NO_XML

// Enable minimal GUI support
#define QT_GUI_LIB
#define QT_WIDGETS_LIB

// Platform string
#define QT_PLATFORM_STR "zephyr"

#endif // QPLATFORMDEFS_ZEPHYR_H