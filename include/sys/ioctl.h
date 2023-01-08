#ifndef _SYS_IOCTL_H_
#define _SYS_IOCTL_H_

#include <sys/ttycom.h>

#ifndef _KERNEL

#include <sys/cdefs.h>

__BEGIN_DECLS
int ioctl(int, unsigned long, ...);
__END_DECLS
#endif /* !_KERNEL */

#endif /* !_SYS_IOCTL_H_ */
