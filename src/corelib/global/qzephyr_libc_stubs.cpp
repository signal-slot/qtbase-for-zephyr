// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*
 * Zephyr libc / POSIX stub TU for Qt6 Core.
 *
 * Builds only when CMAKE_SYSTEM_NAME == "Zephyr".  Provides weak
 * implementations of libc / POSIX functions that:
 *   - Qt's Unix code paths reference unconditionally
 *   - Zephyr SDK newlib (bare-metal) does not ship as defined symbols
 *
 * These stubs land in libQt6Core.a so every Qt-on-Zephyr application
 * picks them up automatically -- there is no need for per-app
 * posix_stubs.c shims.  All symbols are weak so the strong
 * implementations Zephyr's POSIX subsystem provides at Stage 2 (with
 * CONFIG_POSIX_API=y) win the link, and the host build's real libc
 * wins on the host X11-sim path.
 *
 * Two classes of stub live here:
 *
 *  - File-system / identity stubs (getuid, gethostname, truncate,
 *    flock, ...) used by QFileSystemEngine on Unix paths Qt-on-Zephyr
 *    doesn't truly support.  They return -1/ENOSYS.
 *
 *  - V4 / WTF memory-allocator stubs (mmap, munmap, mprotect, sysconf)
 *    used by qtdeclarative's WTF::OSAllocator and PageBlock.  Real V4
 *    code needs them to return success and produce usable memory; we
 *    back mmap with malloc-style allocation, which is OK on Zephyr
 *    because there is no MMU and CONFIG_HEAP_MEM_POOL_SIZE is sized to
 *    fit the QML engine + scene-graph anyway.
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

#define STUB_FAIL do { errno = ENOSYS; return -1; } while (0)

extern "C" {

/* ----- identity / fs ---------------------------------------------- */

__attribute__((weak)) int gethostname(char *name, size_t len)
{ (void)name; (void)len; STUB_FAIL; }

/* QCoreApplication on Q_OS_UNIX calls getuid()/geteuid()/getgid() to
 * decide whether the process is root.  Report a benign non-root uid/gid
 * so the privilege-check branches stay benign. */
__attribute__((weak)) uid_t getuid(void)   { return 1000; }
__attribute__((weak)) uid_t geteuid(void)  { return 1000; }
__attribute__((weak)) gid_t getgid(void)   { return 1000; }
__attribute__((weak)) gid_t getegid(void)  { return 1000; }

__attribute__((weak)) int truncate(const char *p, off_t len)
{ (void)p; (void)len; STUB_FAIL; }
__attribute__((weak)) int flock(int fd, int op)
{ (void)fd; (void)op; STUB_FAIL; }
__attribute__((weak)) int fchmod(int fd, mode_t m)
{ (void)fd; (void)m; STUB_FAIL; }
__attribute__((weak)) int chmod(const char *p, mode_t m)
{ (void)p; (void)m; STUB_FAIL; }
__attribute__((weak)) int access(const char *p, int m)
{ (void)p; (void)m; STUB_FAIL; }
__attribute__((weak)) int link(const char *o, const char *n)
{ (void)o; (void)n; STUB_FAIL; }
__attribute__((weak)) int symlink(const char *t, const char *l)
{ (void)t; (void)l; STUB_FAIL; }
__attribute__((weak)) ssize_t readlink(const char *p, char *b, size_t s)
{ (void)p; (void)b; (void)s; STUB_FAIL; }
__attribute__((weak)) char *realpath(const char *p, char *r)
{ (void)p; (void)r; errno = ENOSYS; return NULL; }
__attribute__((weak)) char *getcwd(char *b, size_t s)
{ (void)b; (void)s; errno = ENOSYS; return NULL; }

/* passwd / group: Picolibc's <pwd.h> / <grp.h> are not consistently on
 * the include path during Stage 1.  Forward-declare the structs locally
 * (only the pointers are taken, no field access). */
struct passwd; struct group;
__attribute__((weak)) struct passwd *getpwuid(uid_t uid)
{ (void)uid; errno = ENOSYS; return NULL; }
__attribute__((weak)) struct group  *getgrgid(gid_t gid)
{ (void)gid; errno = ENOSYS; return NULL; }

/* ----- sysconf weak fallback -------------------------------------- */

/* sysconf(_SC_PAGESIZE) is queried by WTF::pageSize() on rare paths.
 * Zephyr's newlib declares sysconf() in <sys/unistd.h> but bare-metal
 * libnosys leaves the body undefined.  Weak fallback returns 4 KB for
 * _SC_PAGESIZE; Zephyr's strong sysconf wins the link if
 * CONFIG_POSIX_API=y supplies one. */
#ifndef _SC_PAGESIZE
#  define _SC_PAGESIZE 8
#endif
__attribute__((weak)) long sysconf(int name)
{
    if (name == _SC_PAGESIZE)
        return 4096;
    errno = EINVAL;
    return -1;
}

/* NOTE: earlier revisions of this file shipped *strong* overrides of
 * mmap / munmap / mprotect to work around Zephyr's MMU-less POSIX
 * mmap unconditionally returning MAP_FAILED.  Those overrides
 * collided with Zephyr's POSIX mmap at startup and produced MPU
 * stacking faults during C++ static init, so they have been removed.
 * The V4 allocator now bypasses mmap entirely on Zephyr: see
 * qtdeclarative/src/3rdparty/masm/wtf/OSAllocatorPosix.cpp, the
 * OS(ZEPHYR) branches that call posix_memalign() directly. */

/* ----- 64-bit __atomic_* libcalls -------------------------------- */

/* Cortex-M7 has 32-bit ldrex/strex but no 64-bit atomic instructions
 * natively, so GCC emits libcalls (__atomic_*_8) for std::atomic<int64>
 * etc.  The Zephyr SDK does not ship libatomic, leaving the symbols
 * unresolved -- and --unresolved-symbols=ignore-all then NULL-points
 * the calls, which V4 / QML promptly faults on.
 *
 * The functions below provide a complete __atomic_*_8 family.  They
 * are atomic against IRQs on a single-core Cortex-M because each one
 * brackets the underlying read-modify-write with PRIMASK manipulation
 * (cpsid i / cpsie i).  This is the same primitive Zephyr's
 * arch_irq_lock / arch_irq_unlock use on Cortex-M; we open-code the
 * inline assembly here so this TU stays buildable in Stage 1 (where
 * <zephyr/kernel.h> and <zephyr/irq.h> are not yet available).
 *
 * Note: still NOT safe across multiple cores -- this Cortex-M7 build
 * runs on a single CPU.  If a multi-core variant is ever targeted,
 * either link a real libatomic or replace these with LDREXD/STREXD
 * loops.
 *
 * All implementations are weak so a real libatomic, if later linked,
 * overrides them. */

#include <stdint.h>

#if defined(__arm__) || defined(__aarch64__)
static inline unsigned int qzephyr_atomic_lock(void)
{
    unsigned int key;
    __asm__ volatile ("mrs %0, PRIMASK\n\tcpsid i"
                      : "=r" (key) :: "memory");
    return key;
}
static inline void qzephyr_atomic_unlock(unsigned int key)
{
    __asm__ volatile ("msr PRIMASK, %0" :: "r" (key) : "memory");
}
#else
/* Stage 1 builds with the arm-zephyr-eabi toolchain so this branch is
 * not reached in practice; keep it as a clear compile-time error
 * marker if anyone ports to a non-ARM core without revisiting these
 * primitives. */
#  error "qzephyr_libc_stubs: 64-bit __atomic_*_8 stubs need IRQ-mask primitive for this arch"
#endif

#define QZ_ATOMIC_LOAD_STORE(BITS, T)                                            \
__attribute__((weak)) T __atomic_load_##BITS(const volatile void *ptr, int)      \
{                                                                                \
    unsigned int k = qzephyr_atomic_lock();                                      \
    T v = *(const volatile T *)ptr;                                              \
    qzephyr_atomic_unlock(k);                                                    \
    return v;                                                                    \
}                                                                                \
__attribute__((weak)) void __atomic_store_##BITS(volatile void *ptr, T val, int) \
{                                                                                \
    unsigned int k = qzephyr_atomic_lock();                                      \
    *(volatile T *)ptr = val;                                                    \
    qzephyr_atomic_unlock(k);                                                    \
}                                                                                \
__attribute__((weak)) T __atomic_exchange_##BITS(volatile void *ptr, T val, int) \
{                                                                                \
    unsigned int k = qzephyr_atomic_lock();                                      \
    T old = *(volatile T *)ptr;                                                  \
    *(volatile T *)ptr = val;                                                    \
    qzephyr_atomic_unlock(k);                                                    \
    return old;                                                                  \
}                                                                                \
__attribute__((weak)) bool __atomic_compare_exchange_##BITS(                     \
        volatile void *ptr, void *expected, T desired,                           \
        bool /*weak*/, int /*succ*/, int /*fail*/)                               \
{                                                                                \
    unsigned int k = qzephyr_atomic_lock();                                      \
    T cur = *(volatile T *)ptr;                                                  \
    T exp = *(T *)expected;                                                      \
    bool eq = (cur == exp);                                                      \
    if (eq)                                                                      \
        *(volatile T *)ptr = desired;                                            \
    else                                                                         \
        *(T *)expected = cur;                                                    \
    qzephyr_atomic_unlock(k);                                                    \
    return eq;                                                                   \
}

/* RMW: EXPR uses local names `old` (current value at ptr) and `val`
 * (the operand passed in).  Result of EXPR becomes the new stored
 * value; fetch_OP returns the pre-op `old`, OP_fetch returns EXPR. */
#define QZ_ATOMIC_RMW(BITS, T, NAME, EXPR)                                       \
__attribute__((weak)) T __atomic_fetch_##NAME##_##BITS(volatile void *ptr, T val, int) \
{                                                                                \
    unsigned int k = qzephyr_atomic_lock();                                      \
    T old = *(volatile T *)ptr;                                                  \
    *(volatile T *)ptr = EXPR;                                                   \
    qzephyr_atomic_unlock(k);                                                    \
    return old;                                                                  \
}                                                                                \
__attribute__((weak)) T __atomic_##NAME##_fetch_##BITS(volatile void *ptr, T val, int) \
{                                                                                \
    unsigned int k = qzephyr_atomic_lock();                                      \
    T old = *(volatile T *)ptr;                                                  \
    T nv = EXPR;                                                                 \
    *(volatile T *)ptr = nv;                                                     \
    qzephyr_atomic_unlock(k);                                                    \
    return nv;                                                                   \
}

QZ_ATOMIC_LOAD_STORE(8, uint64_t)

QZ_ATOMIC_RMW(8, uint64_t, add,  (uint64_t)(old + val))
QZ_ATOMIC_RMW(8, uint64_t, sub,  (uint64_t)(old - val))
QZ_ATOMIC_RMW(8, uint64_t, and,  (uint64_t)(old & val))
QZ_ATOMIC_RMW(8, uint64_t, or,   (uint64_t)(old | val))
QZ_ATOMIC_RMW(8, uint64_t, xor,  (uint64_t)(old ^ val))
QZ_ATOMIC_RMW(8, uint64_t, nand, (uint64_t)~(old & val))

#undef QZ_ATOMIC_LOAD_STORE
#undef QZ_ATOMIC_RMW

} /* extern "C" */
