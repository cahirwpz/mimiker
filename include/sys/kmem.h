#ifndef _SYS_KMEM_H_
#define _SYS_KMEM_H_

#include <sys/kmem_flags.h>

typedef struct vm_page vm_page_t;

/* Initializes kernel virtual address space allocator & manager. */
void init_kmem(void);

/*
 * Kernel page-sized memory allocator.
 */

void *kmem_alloc(size_t size, kmem_flags_t flags) __warn_unused;
void kmem_free(void *ptr, size_t size);

/* Allocates contiguous physical memory of `size` bytes aligned to at least
 * `PAGESIZE` boundary. First physical address of the region will be stored
 * under `pap`. Memory will be mapped read-write with `flags` passed to
 * `pmap_kenter`.
 *
 * Returns kernel virtual address where the memory is mapped.
 */
vaddr_t kmem_alloc_contig(paddr_t *pap, size_t size,
                          unsigned flags) __warn_unused;

/* Map contiguous physical memory of `size` bytes starting from `pa` address.
 * Memory will be mapped read-write with `flags` passed to `pmap_kenter`.
 *
 * Returns kernel virtual address where the memory is mapped. */
vaddr_t kmem_map_contig(paddr_t pa, size_t size, unsigned flags) __warn_unused;

/*
 * Kernel virtual address space allocator.
 */

/* Allocates a range of kernel virtual address space of `size` pages (in bytes)
 * and returns virtual address. kva_alloc never fails. */
vaddr_t kva_alloc(size_t size);
void kva_free(vaddr_t va, size_t size);
vm_page_t *kva_find_page(vaddr_t ptr);

/*
 * Allocates and deallocates memory in kernel virtual address space.
 */
void kva_map(vaddr_t va, size_t size, kmem_flags_t flags);
void kva_unmap(vaddr_t va, size_t size);

#endif /* !_SYS_KMEM_H_ */
