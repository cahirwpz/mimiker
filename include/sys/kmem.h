#ifndef _SYS_KMEM_H_
#define _SYS_KMEM_H_

#include <sys/kmem_flags.h>

typedef struct vm_page vm_page_t;

/*! \brief Called during kernel initialization. */
void init_kmem(void);

void *kmem_alloc(size_t size, kmem_flags_t flags) __warn_unused;

/*! \brief Map consecutive physical pages with given pmap flags. */
vaddr_t kmem_map(paddr_t pa, size_t size, unsigned flags) __warn_unused;
void kmem_free(void *ptr, size_t size);

/* Allocates contiguous physical memory of `size` pages (in bytes) aligned to at
 * least `PAGESIZE` boundary. The memory will be mapped under address stored
 * under `vap`. First physical address of the region will be stored under `pap`.
 * Memory will be mapped read-write with PMAP_NOCACHE flag.
 *
 * Returns ENOMEM if could not find enough contiguous physical memory to
 * satisfy the request. */
int kmem_alloc_contig(size_t size, vaddr_t *vap, paddr_t *pap);

/* Kernel virtual address space allocator. */
vaddr_t kva_alloc(size_t size);
void kva_free(vaddr_t ptr, size_t size);
vm_page_t *kva_find_page(vaddr_t ptr);

void kva_map(vaddr_t ptr, size_t size, kmem_flags_t flags);
void kva_unmap(vaddr_t ptr, size_t size);

#endif /* !_SYS_KMEM_H_ */
