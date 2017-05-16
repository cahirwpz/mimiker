#ifndef _SYS_MMAP_H_
#define _SYS_MMAP_H_

#include <vm.h>
#include <sysent.h>

#define MMAP_FLAG_ANONYMOUS 1
/* TODO: Other flags that may be simple to implement: FIXED,
   UNINITIALIZED, GROWSDOWN. */

#define MMAP_FAILED ((vm_addr_t)-1)

/* mmap will never consider allocating memory below this address. */
#define MMAP_LOW_ADDR 0x00400000

vm_addr_t do_mmap(vm_addr_t addr, size_t length, vm_prot_t prot, int flags,
                  int *error);
int do_munmap(vm_addr_t addr, size_t length);

#endif /* !_SYS_MMAP_H_ */
