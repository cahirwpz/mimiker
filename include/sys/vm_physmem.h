#ifndef _SYS_VM_PHYSMEM_H_
#define _SYS_VM_PHYSMEM_H_

#include <sys/vm.h>

typedef struct vm_physseg vm_physseg_t;

/* \brief Allocate vm_page structures to be managed by vm_physseg allocator. */
void init_vm_page(void);

/* \brief Allocate physical memory segment without vm_page structures. */
#define vm_physseg_plug(start, end) _vm_physseg_plug((start), (end), false)
#define vm_physseg_plug_used(start, end) _vm_physseg_plug((start), (end), true)
void _vm_physseg_plug(paddr_t start, paddr_t end, bool used);

/* Allocates contiguous big page that consists of n machine pages. */
vm_page_t *vm_page_alloc(size_t n);

/* Allocates `n` pages in various sizes and puts them on `pglist`. Always
 * initializes `pglist`. Returns ENOMEM if the request cannot be satisfied. */
int vm_pagelist_alloc(size_t n, vm_pagelist_t *pglist);

/* Releases all pages on `pglist`. */
void vm_pagelist_free(vm_pagelist_t *pglist);

/* Returns vm_page associated with frame of given address. */
vm_page_t *vm_page_find(paddr_t pa);

/* Returns vm_page to physical memory manager. */
void vm_page_free(vm_page_t *page);

#endif /* !_SYS_VM_PHYSMEM_H_ */
