#ifndef _SYS_PARAM_H_
#define _SYS_PARAM_H_

#include <sys/syslimits.h>
#include <sys/inttypes.h>

/* Macros for min/max. */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define MAXPHYS (64 * 1024) /* max raw I/O transfer size */

/*
 * Round p (pointer or byte index) up to a correctly-aligned value for all
 * data types (int, long, ...).   The result is u_int and must be cast to
 * any desired pointer type.
 *
 * ALIGNED_POINTER is a boolean macro that checks whether an address
 * is valid to fetch data elements of type t from on this architecture.
 * This does not reflect the optimal alignment, just the possibility
 * (within reasonable limits).
 *
 */
#define ALIGNBYTES __ALIGNBYTES
#ifndef ALIGN
#define ALIGN(p) (((uintptr_t)(p) + ALIGNBYTES) & ~ALIGNBYTES)
#endif
#ifndef ALIGNED_POINTER
#define ALIGNED_POINTER(p, t) ((((uintptr_t)(p)) & (sizeof(t) - 1)) == 0)
#endif

/*
 * Miscellaneous.
 */
#define CMASK 022 /* default file mask: S_IWGRP|S_IWOTH */

/*
 * File system parameters and macros.
 *
 * The file system is made out of blocks of at most MAXBSIZE units, with
 * smaller units (fragments) only in the last direct block.  MAXBSIZE
 * primarily determines the size of buffers in the buffer pool.  It may be
 * made larger without any effect on existing file systems; however making
 * it smaller may make some file systems unmountable.
 */
#define MAXBSIZE MAXPHYS
#define MAXFRAG 8

/*
 * MAXPATHLEN defines the longest permissible path length after expanding
 * symbolic links. It is used to allocate a temporary buffer from the buffer
 * pool in which to do the name expansion, hence should be a power of two,
 * and must be less than or equal to MAXBSIZE.  MAXSYMLINKS defines the
 * maximum number of symbolic links that may be expanded in a path name.
 * It should be set high enough to allow all legitimate uses, but halt
 * infinite loops reasonably quickly.
 *
 * MAXSYMLINKS should be >= _POSIX_SYMLOOP_MAX (see <limits.h>)
 */
#define MAXPATHLEN PATH_MAX
#define MAXSYMLINKS 32

/* Bit map related macros. */
#define setbit(a, i) ((a)[(i) / NBBY] |= 1 << ((i) % NBBY))
#define clrbit(a, i) ((a)[(i) / NBBY] &= ~(1 << ((i) % NBBY)))
#define isset(a, i) ((a)[(i) / NBBY] & (1 << ((i) % NBBY)))
#define isclr(a, i) (((a)[(i) / NBBY] & (1 << ((i) % NBBY))) == 0)

/* Macros for counting and rounding. */
#ifndef howmany
#define howmany(x, y) (((x) + ((y)-1)) / (y))
#endif
#define roundup(x, y) ((((x) + ((y)-1)) / (y)) * (y))
#define rounddown(x, y) (((x) / (y)) * (y))

/*
 * Rounding to powers of two.  The naive definitions of roundup2 and
 * rounddown2,
 *
 *	#define	roundup2(x,m)	(((x) + ((m) - 1)) & ~((m) - 1))
 *	#define	rounddown2(x,m)	((x) & ~((m) - 1)),
 *
 * exhibit a quirk of integer arithmetic in C because the complement
 * happens in the type of m, not in the type of x.  So if unsigned int
 * is 32-bit, and m is an unsigned int while x is a uint64_t, then
 * roundup2 and rounddown2 would have the unintended effect of clearing
 * the upper 32 bits of the result(!).  These definitions avoid the
 * pitfalls of C arithmetic depending on the types of x and m, and
 * additionally avoid multiply evaluating their arguments.
 */
#define roundup2(x, m) ((((x)-1) | ((m)-1)) + 1)
#define rounddown2(x, m) ((x) & ~((__typeof__(x))((m)-1)))

#define powerof2(x) ((((x)-1) & (x)) == 0)

#define NGROUPS NGROUPS_MAX

/* Signals. */
#ifndef _KERNEL
#include <sys/signal.h>
#endif

#endif /* !_SYS_PARAM_H_ */
