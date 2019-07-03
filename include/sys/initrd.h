#ifndef _SYS_INITRD_H_
#define _SYS_INITRD_H_

#include <sys/cdefs.h>

/* Following getters are defined by platform bootstrap code. */
intptr_t ramdisk_get_start(void);
size_t ramdisk_get_size(void);

void ramdisk_dump(void);

#endif /* !_SYS_INITRD_H_ */
