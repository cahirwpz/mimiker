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
void dtb_early_init(paddr_t pa, vaddr_t va);

/* Initialize dtb subsystem from MI code; nop if dtb is not supported. */
void dtb_init(void);

/* Read a single pair of the form <addr size>. */
void dtb_pair(int parent, int node, const char *name, unsigned long *addr_p,
              unsigned long *size_p);

/* Read an address-only property (<addr>). */
unsigned long dtb_addr(int parent, int node, const char *name);

/* Read a single cell property (<value>). */
unsigned dtb_cell(int node, const char *name);

/* Read a property of the cpus node. */
unsigned dtb_cpus_prop(const char *name);

/* Read a string property. */
const char *dtb_str(int node, const char *name);

/* Get region property of `node`. */
void dtb_reg(int parent, int node, unsigned long *addr_p,
             unsigned long *size_p);

/* Get physical memory boundaries. */
void dtb_mem(unsigned long *addr_p, unsigned long *size_p);

/* Get reserved memory boundaries. */
void dtb_rsvdmem(unsigned long *addr_p, unsigned long *size_p);

/* Get initrd memory boundaries. */
void dtb_rd(unsigned long *addr_p, unsigned long *size_p);

/* Get kernel boot arguments. */
const char *dtb_cmdline(void);

#endif /* !_SYS_DTB_ */
