// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMDEFS_H
#define QPLATFORMDEFS_H

// Get Qt defines/settings
#include "qglobal.h"

// Set any POSIX/XOPEN defines at the top of this file to turn on specific APIs
#include <unistd.h>

// We are hot - unistd.h should have turned on the specific APIs we requested

// Zephyr RTOS specific includes
#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/semaphore.h>
#include <zephyr/posix/time.h>
#include <zephyr/posix/signal.h>
#include <zephyr/posix/sys/stat.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/dirent.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

// Zephyr doesn't have these by default
#ifndef PATH_MAX
#define PATH_MAX 256
#endif

#define QT_USE_XOPEN_LFS_EXTENSIONS
#include "../posix/qplatformdefs.h"

// Zephyr specific overrides
#undef QT_OPEN_LARGEFILE
#define QT_OPEN_LARGEFILE       0

// Thread support
#define QT_NO_THREAD_PRIORITY

// File system limitations
#define QT_NO_FSFILEENGINE
#define QT_NO_TEMPORARYFILE

// Network limitations  
#define QT_NO_NETWORKINTERFACE

// Process limitations
#define QT_NO_PROCESS

// Library loading limitations
#define QT_NO_LIBRARY

#endif // QPLATFORMDEFS_H