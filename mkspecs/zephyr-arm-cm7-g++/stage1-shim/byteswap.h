/* Stage 1 stub: <byteswap.h>
 *
 * Linux/glibc provides bswap_16/32/64 inline functions.  Newlib does
 * not.  Qt's bundled sha3 brg_endian.h pulls this in transitively
 * after <endian.h>.  Forward to GCC builtins.
 */
#ifndef QZEPHYR_STAGE1_BYTESWAP_H
#define QZEPHYR_STAGE1_BYTESWAP_H

#include <stdint.h>

#define bswap_16(x) __builtin_bswap16((uint16_t)(x))
#define bswap_32(x) __builtin_bswap32((uint32_t)(x))
#define bswap_64(x) __builtin_bswap64((uint64_t)(x))

#endif /* QZEPHYR_STAGE1_BYTESWAP_H */
