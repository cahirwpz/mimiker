#ifndef _SYS_MMAN_H_
#define _SYS_MMAN_H_

#include <stdint.h>

#define MMAP_ANON 1
/* TODO: Other flags that may be simple to implement:
 * FIXED, UNINITIALIZED, GROWSDOWN. */
#define MMAP_FAILED ((intptr_t)-1)

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

#ifndef _KERNELSPACE

/* Newlib does not provide mmap prototype, so we need to use our own. */
void *mmap(void *addr, size_t length, int prot, int flags);

#else /* _KERNELSPACE */

#include <vm.h>

/* mmap will never consider allocating memory below this address. */
#define MMAP_LOW_ADDR 0x00400000

vm_addr_t do_mmap(vm_addr_t addr, size_t length, vm_prot_t prot, int flags,
                  int *error);
int do_munmap(vm_addr_t addr, size_t length);

#endif /* !_KERNELSPACE */

#endif /* !_SYS_MMAN_H_ */
