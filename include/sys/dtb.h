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

/* Return size of dtb. */
size_t dtb_size(void);

/* Inittialize dtb subsystem from boot code. */
void dtb_early_init(paddr_t pa, vaddr_t va, size_t size);

/* Initialize dtb subsystem from MI code; nop if dtb is not supported. */
void dtb_init(void);

/* Return offset of path at dtb or die. */
int dtb_offset(const char *path);

/* Get region property of the given node. */
void dtb_reg(int node, const uint32_t **prop_p, int *len_p);

/* Get physical memory boundaries. */
void dtb_mem(uint32_t *start_p, uint32_t *size_p);

/* Get reserved memory boundaries. */
void dtb_memrsvd(uint32_t *start_p, uint32_t *size_p);

/* Get initrd memory boundaries. */
void dtb_rd(uint32_t *start_p, uint32_t *size_p);

/* Get kernel boot arguments. */
const char *dtb_cmdline(void);

#endif /* !_SYS_DTB_ */
