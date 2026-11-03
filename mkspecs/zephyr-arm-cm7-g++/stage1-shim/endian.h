/* Stage 1 stub: <endian.h>
 *
 * Linux/glibc provides <endian.h> with htobeXX / htoleXX byte-order
 * macros.  Newlib bare-metal does not (picolibc has its own copy; we
 * don't mix libcs).  Qt's bundled sha3 brg_endian.h transitively
 * pulls this in.  Provide a minimal version for Stage 1 -- on ARM
 * we know we're little-endian.
 */
#ifndef QZEPHYR_STAGE1_ENDIAN_H
#define QZEPHYR_STAGE1_ENDIAN_H

#include <stdint.h>

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __PDP_ENDIAN    3412

#ifndef __BYTE_ORDER
#  define __BYTE_ORDER __LITTLE_ENDIAN  /* ARM Cortex-M is LE in our config */
#endif

#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BIG_ENDIAN    __BIG_ENDIAN
#define PDP_ENDIAN    __PDP_ENDIAN
#define BYTE_ORDER    __BYTE_ORDER

/* GCC has all the swap builtins; use them. */
#define htobe16(x) __builtin_bswap16((uint16_t)(x))
#define htole16(x) ((uint16_t)(x))
#define be16toh(x) __builtin_bswap16((uint16_t)(x))
#define le16toh(x) ((uint16_t)(x))

#define htobe32(x) __builtin_bswap32((uint32_t)(x))
#define htole32(x) ((uint32_t)(x))
#define be32toh(x) __builtin_bswap32((uint32_t)(x))
#define le32toh(x) ((uint32_t)(x))

#define htobe64(x) __builtin_bswap64((uint64_t)(x))
#define htole64(x) ((uint64_t)(x))
#define be64toh(x) __builtin_bswap64((uint64_t)(x))
#define le64toh(x) ((uint64_t)(x))

#endif /* QZEPHYR_STAGE1_ENDIAN_H */
