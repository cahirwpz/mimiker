#ifndef _SYS_INITRD_H_
#define _SYS_INITRD_H_

#include <common.h>

intptr_t ramdisk_get_start(void);
unsigned ramdisk_get_size(void);

void ramdisk_dump(void);

#endif /* !_SYS_INITRD_H_ */
