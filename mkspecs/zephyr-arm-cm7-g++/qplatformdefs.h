// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMDEFS_H
#define QPLATFORMDEFS_H

// Stage 1 platform definitions for Cortex-M7 / Zephyr.  Provides the
// minimum POSIX-like surface Qt needs at compile time, sourced from the
// newlib bundled with the Zephyr SDK.  Deliberately does NOT include
// <zephyr/kernel.h> or <zephyr/posix/*> -- those headers depend on the
// Kconfig-driven autoconf.h that only exists inside `west build`, so
// pulling them in here breaks every Qt translation unit.  All code that
// genuinely needs Zephyr runtime APIs (corelib's qeventdispatcher_zephyr,
// the qzephyr QPA plugin) is excluded from the Stage 1 build via
// QT_DEFER_ZEPHYR_RUNTIME and compiled later inside the Zephyr
// application's CMake context.

#include "qglobal.h"

// Note: do NOT define QT_NO_FSFILEENGINE / QT_NO_TEMPORARYFILE etc. here.
// Those macros also hide class definitions in qfsfileengine_p.h /
// qtemporaryfile_p.h that other Qt corelib translation units reference as
// concrete types (std::unique_ptr<QAbstractFileEngine> needs the
// destructor visible, etc.).  Disable the corresponding *runtime* code
// through Qt's FEATURE_* flags in the build script -- those gate the
// .cpp compilation without erasing the class declarations.

// Minimum standard library headers (provided by newlib in Zephyr SDK).
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>  // DIR / opendir / closedir / readdir (needed by common/posix/qplatformdefs.h)

// Some constants that Qt expects from a fuller POSIX environment.
#ifndef PATH_MAX
#  define PATH_MAX 256
#endif

// newlib for bare-metal ARM does not ship <sys/socket.h>; tell
// common/posix/qplatformdefs.h to skip that include.
#define QT_NO_SOCKET_H

// Do NOT define QT_USE_XOPEN_LFS_EXTENSIONS: newlib bare-metal lacks
// lseek64/open64/fstat64/etc.; we use the plain 32-bit variants.
#include "../common/posix/qplatformdefs.h"

#undef QT_OPEN_LARGEFILE
#define QT_OPEN_LARGEFILE       0

// newlib bare-metal hides lstat() behind __SPU__/__rtems__/__CYGWIN__.
// On Zephyr we don't have symlinks, so alias QT_LSTAT to plain stat().
#undef QT_LSTAT
#define QT_LSTAT                ::stat

// BSD/Linux-only legacy APIs that Qt corelib reaches for unconditionally
// but newlib ARM bare-metal does not declare.  We provide prototype-only
// declarations so Stage 1 source files compile; Stage 2 / the Zephyr app
// build is where real bodies (or further stubs) get linked in.
#ifdef __cplusplus
extern "C" {
#endif
int getpagesize(void);
int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);
#ifndef AT_FDCWD
#  define AT_FDCWD (-100)
#endif
#ifdef __cplusplus
}
#endif

#endif // QPLATFORMDEFS_H
