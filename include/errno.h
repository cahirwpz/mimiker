#ifndef _ERRNO_H_
#define _ERRNO_H_

#include <sys/cdefs.h>
#include <sys/errno.h>

__BEGIN_DECLS

#ifndef __errno
int *__errno(void);
#define __errno __errno
#endif

#ifndef errno
#define errno (*__errno())
#endif

extern const int sys_nerr;
extern const char *const sys_errlist[];
__END_DECLS

#endif /* !_ERRNO_H_ */
