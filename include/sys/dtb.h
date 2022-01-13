#ifndef _SYS_DTB_
#define _SYS_DTB_

#include <sys/types.h>

#define DTB_ROOT_NODE 0

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
void dtb_early_init(paddr_t pa, vaddr_t va);

/* Initialize dtb subsystem from MI code; nop if dtb is not supported. */
void dtb_init(void);

/* Get region property of the given node. */
void dtb_reg(int parent, int node, uint64_t *addr_p, uint64_t *size_p);

/* Get a single cell property. */
uint64_t dtb_cell(int parent, int node, const char *name);

/* Get physical memory boundaries. */
void dtb_mem(uint64_t *addr_p, uint64_t *size_p);

/* Get reserved memory boundaries. */
void dtb_rsvdmem(uint64_t *addr_p, uint64_t *size_p);

/* Get initrd memory boundaries. */
void dtb_rd(uint64_t *addr_p, uint64_t *size_p);

/* Get kernel boot arguments. */
const char *dtb_cmdline(void);

#endif /* !_SYS_DTB_ */
