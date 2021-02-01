#ifndef _SYS_PTY_H_
#define _SYS_PTY_H_

#include <sys/mimiker.h>
#include <sys/proc.h>

/* Implementation of posix_openpt(), as specified by POSIX. */
int do_posix_openpt(proc_t *p, int flags, register_t *res);

#endif /* !_SYS_PTY_H_ */
