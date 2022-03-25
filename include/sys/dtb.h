#ifndef _SYS_DTB_
#define _SYS_DTB_

#include <sys/types.h>

/*
 * NOTE: for dtb parser look at sys/fdt.h.
 */

/* Return handle (va) to dtb provided by bootloader. */
void *dtb_root(void);

/*
 * Return physical address of dtb. That address should be provided by
 * bootlader.
 */
paddr_t dtb_early_root(void);

/* Inittialize dtb subsystem from boot code. */
void dtb_early_init(paddr_t dtb, size_t size);

/* Initialize dtb subsystem from MI code; nop if dtb is not supported. */
void dtb_init(void);

#endif /* !_SYS_DTB_ */
