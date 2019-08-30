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

#endif /* __ASSEMBLER__ */

#endif /* !_SYS_ENDIAN_H_ */
