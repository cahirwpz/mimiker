#ifndef _SYS_KMEM_H_
#define _SYS_KMEM_H_

#include <sys/kmem_flags.h>

void kmem_bootstrap(void);
void *kmem_alloc(size_t size, kmem_flags_t flags) __warn_unused;
void *kmem_map(paddr_t pa, size_t size) __warn_unused;
void kmem_free(void *ptr, size_t size);

/* Kernel virtual address space allocator. */
vaddr_t kva_alloc(size_t size);
void kva_free(vaddr_t ptr, size_t size);

#endif /* !_SYS_KMEM_H_ */
