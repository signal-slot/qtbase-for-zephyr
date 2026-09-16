/* Stage 1 stub: <sys/mman.h>
 *
 * Zephyr SDK's newlib/picolibc does not ship <sys/mman.h>.  Qt's
 * qresource.cpp includes it for mmap()/munmap() when reading on-disk
 * resource binaries via memory mapping.  On a Zephyr target the
 * resource path is generally redirected to embedded byte arrays
 * (FEATURE_resource is left on but mmap() is not actually invoked when
 * resources are statically compiled in), so a header-only stub is
 * enough to compile.  Stage 2 (west build) provides the real
 * implementation if mmap-backed resources are ever needed at runtime.
 */
#ifndef QZEPHYR_STAGE1_SYS_MMAN_H
#define QZEPHYR_STAGE1_SYS_MMAN_H

#include <sys/types.h>

#define PROT_NONE       0x0
#define PROT_READ       0x1
#define PROT_WRITE      0x2
#define PROT_EXEC       0x4

#define MAP_SHARED      0x01
#define MAP_PRIVATE     0x02
#define MAP_FIXED       0x10
#define MAP_ANONYMOUS   0x20
#define MAP_ANON        MAP_ANONYMOUS

#define MAP_FAILED ((void *) -1)

#ifdef __cplusplus
extern "C" {
#endif

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);

/* qtdeclarative's masm/stubs/ExecutableAllocator.h calls mprotect() to
 * flip code pages between W and X.  The Cortex-M7 build has the JIT
 * disabled (FEATURE_qml_jit=OFF) so this is dead code, but the symbol
 * declaration still has to be visible to the preprocessor.
 */
int mprotect(void *addr, size_t len, int prot);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_SYS_MMAN_H */
