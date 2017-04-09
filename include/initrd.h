#ifndef _SYS_INITRD_H_
#define _SYS_INITRD_H_

#include <common.h>

intptr_t ramdisk_get_start();
unsigned ramdisk_get_size();

void ramdisk_init();
void ramdisk_dump();

#endif /* !_SYS_INITRD_H_ */
