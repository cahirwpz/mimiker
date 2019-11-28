#ifndef _SYS_VM_PHYSMEM_H_
#define _SYS_VM_PHYSMEM_H_

#include <sys/vm.h>

typedef struct vm_physseg vm_physseg_t;

/* \brief Allocate vm_page structures to be managed by vm_physseg allocator. */
void vm_page_init(void);

/* \brief Allocate physical memory segment without vm_page structures. */
void vm_physseg_plug(paddr_t start, paddr_t end);

/* Allocates contiguous big page that consists of n machine pages. */
vm_page_t *vm_page_alloc(size_t n);

/* Returns vm_page associated with frame of given address. */
vm_page_t *vm_page_find(paddr_t pa);

/* Returns vm_page to physical memory manager. */
void vm_page_free(vm_page_t *page);

#endif /* !_SYS_VM_PHYSMEM_H_ */
