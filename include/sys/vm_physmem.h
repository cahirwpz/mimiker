#ifndef _SYS_VM_PHYSMEM_H_
#define _SYS_VM_PHYSMEM_H_

#include <sys/vm.h>

typedef struct vm_physseg vm_physseg_t;

/* \brief Allocate vm_page structures to be managed by vm_physseg allocator. */
void vm_page_init(void);

/* \brief Allocate physical memory segment without vm_page structures. */
vm_physseg_t *vm_physseg_alloc(paddr_t start, paddr_t end);

/* \brief After using this function pages in range (start, end) are never going
 * to be allocated. Should be used at start to avoid allocating from text, data,
 * ebss, or any possibly unwanted places. */
void vm_physseg_reserve(vm_physseg_t *seg, paddr_t start, paddr_t end);

/* Allocates contiguous big page that consists of n machine pages. */
vm_page_t *vm_page_alloc(size_t n);

/* Returns vm_page associated with frame of given address. */
vm_page_t *vm_page_find(paddr_t pa);

/* Returns vm_page to physical memory manager. */
void vm_page_free(vm_page_t *page);

#endif /* !_SYS_VM_PHYSMEM_H_ */
