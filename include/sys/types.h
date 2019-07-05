#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#include <endian.h>
#include <stdint.h>

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;

typedef uint64_t u_quad_t;
typedef int64_t quad_t;

typedef int64_t time_t;
typedef uint32_t useconds_t;
typedef int32_t suseconds_t; /* microseconds (signed) */

#endif /* !_SYS_TYPES_H_ */
