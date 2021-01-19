#ifndef _SYS_ENDIAN_H_
#define _SYS_ENDIAN_H_

#ifndef __BYTE_ORDER__
#error __BYTE_ORDER__ has not been defined by the compiler!
#endif

#define _BIG_ENDIAN __ORDER_BIG_ENDIAN__
#define _LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#define _PDP_ENDIAN __ORDER_PDP_ENDIAN__
#define _BYTE_ORDER __BYTE_ORDER__

#define BIG_ENDIAN _BIG_ENDIAN
#define LITTLE_ENDIAN _LITTLE_ENDIAN
#define PDP_ENDIAN _PDP_ENDIAN
#define BYTE_ORDER _BYTE_ORDER

/*
 * Define the order of 32-bit words in 64-bit words.
 */
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define _QUAD_HIGHWORD 1
#define _QUAD_LOWWORD 0
#endif

#if _BYTE_ORDER == _BIG_ENDIAN
#define _QUAD_HIGHWORD 0
#define _QUAD_LOWWORD 1
#endif

#ifndef __ASSEMBLER__

#include <sys/types.h>

/*
 * Macros to convert to a specific endianness.
 */

#if BYTE_ORDER == BIG_ENDIAN

#define htobe16(x) (x)
#define htobe32(x) (x)
#define htobe64(x) (x)
#define htole16(x) __builtin_bswap16((uint16_t)(x))
#define htole32(x) __builtin_bswap32((uint32_t)(x))
#define htole64(x) __builtin_bswap64((uint64_t)(x))

#define HTOBE16(x) (void)(x)
#define HTOBE32(x) (void)(x)
#define HTOBE64(x) (void)(x)
#define HTOLE16(x) (x) = __builtin_bswap16((uint16_t)(x))
#define HTOLE32(x) (x) = __builtin_bswap32((uint32_t)(x))
#define HTOLE64(x) (x) = __builtin_bswap64((uint64_t)(x))

#else /* LITTLE_ENDIAN */

#define htobe16(x) __builtin_bswap16((uint16_t)(x))
#define htobe32(x) __builtin_bswap32((uint32_t)(x))
#define htobe64(x) __builtin_bswap64((uint64_t)(x))
#define htole16(x) (x)
#define htole32(x) (x)
#define htole64(x) (x)

#define HTOBE16(x) (x) = __builtin_bswap16((uint16_t)(x))
#define HTOBE32(x) (x) = __builtin_bswap32((uint32_t)(x))
#define HTOBE64(x) (x) = __builtin_bswap64((uint64_t)(x))
#define HTOLE16(x) (void)(x)
#define HTOLE32(x) (void)(x)
#define HTOLE64(x) (void)(x)

#endif /* LITTLE_ENDIAN */

#define be16toh(x) htobe16(x)
#define be32toh(x) htobe32(x)
#define be64toh(x) htobe64(x)
#define le16toh(x) htole16(x)
#define le32toh(x) htole32(x)
#define le64toh(x) htole64(x)

#define BE16TOH(x) HTOBE16(x)
#define BE32TOH(x) HTOBE32(x)
#define BE64TOH(x) HTOBE64(x)
#define LE16TOH(x) HTOLE16(x)
#define LE32TOH(x) HTOLE32(x)
#define LE64TOH(x) HTOLE64(x)

/*
 * Routines to encode/decode big- and little-endian multi-octet values
 * to/from an octet stream (From NetBSD).
 */
#define __GEN_ENDIAN_ENC(bits, endian)                                         \
  static __inline void __unused endian##bits##enc(void *dst,                   \
                                                  uint##bits##_t u) {          \
    u = hto##endian##bits(u);                                                  \
    __builtin_memcpy(dst, &u, sizeof(u));                                      \
  }

__GEN_ENDIAN_ENC(16, be)
__GEN_ENDIAN_ENC(32, be)
__GEN_ENDIAN_ENC(64, be)
__GEN_ENDIAN_ENC(16, le)
__GEN_ENDIAN_ENC(32, le)
__GEN_ENDIAN_ENC(64, le)
#undef __GEN_ENDIAN_ENC

#define __GEN_ENDIAN_DEC(bits, endian)                                         \
  static __inline uint##bits##_t __unused endian##bits##dec(const void *buf) { \
    uint##bits##_t u;                                                          \
    __builtin_memcpy(&u, buf, sizeof(u));                                      \
    return endian##bits##toh(u);                                               \
  }

__GEN_ENDIAN_DEC(16, be)
__GEN_ENDIAN_DEC(32, be)
__GEN_ENDIAN_DEC(64, be)
__GEN_ENDIAN_DEC(16, le)
__GEN_ENDIAN_DEC(32, le)
__GEN_ENDIAN_DEC(64, le)
#undef __GEN_ENDIAN_DEC

#endif /* __ASSEMBLER__ */

#endif /* !_SYS_ENDIAN_H_ */
