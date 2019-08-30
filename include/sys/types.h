#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#include <sys/cdefs.h>
#include <endian.h>
#include <stdint.h> /* uint*_t, int*_t */
#include <stddef.h> /* offsetof, NULL, ptrdiff_t, size_t, etc. */
#include <machine/types.h>

#define NBBY 8 /* number of bits per byte */

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;

typedef uint64_t u_quad_t;
typedef int64_t quad_t;

typedef int64_t time_t;
typedef uint32_t useconds_t;
typedef int32_t suseconds_t; /* microseconds (signed) */

typedef char *caddr_t; /* core address */

typedef int32_t off_t; /* should it be int64_t? */
typedef intptr_t ssize_t;
typedef int32_t pid_t;
typedef int32_t pgid_t;
typedef uint16_t dev_t;
typedef uint32_t systime_t; /* kept in system clock ticks */
typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef uint32_t mode_t;
typedef uint16_t nlink_t;
typedef uint16_t ino_t;
typedef uint32_t tid_t;
typedef int32_t blkcnt_t;  /* fs block count */
typedef int32_t blksize_t; /* fs optimal block size */

typedef __builtin_va_list __va_list;

#endif /* !_SYS_TYPES_H_ */
