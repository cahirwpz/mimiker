#ifndef _SYS_FCNTL_H_
#define _SYS_FCNTL_H_

/* Always ensure that these are consistent with <stdio.h> and <unistd.h>! */
#ifndef SEEK_SET
#define SEEK_SET 0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END 2 /* set file offset to EOF plus offset */
#endif

/* File open flags as passed to sys_open. */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2

// #ifndef _KERNELSPACE
#include <sys/cdefs.h>

__BEGIN_DECLS
int open(const char *, int, ...);
int creat(const char *, mode_t);
int fcntl(int, int, ...);
__END_DECLS

// #endif /* !_KERNELSPACE */

#endif /* !_SYS_FCNTL_H_ */
