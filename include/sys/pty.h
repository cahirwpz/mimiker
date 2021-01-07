#include <sys/mimiker.h>
#include <sys/proc.h>

/* Implementation of posix_openpt(), as specified by POSIX. */
int do_posix_openpt(proc_t *p, int flags, register_t *res);
