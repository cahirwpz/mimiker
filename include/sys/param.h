#ifndef _SYS_PARAM_H_
#define _SYS_PARAM_H

#include <sys/syslimits.h>

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

#endif /* !_SYS_PARAM_H_ */
