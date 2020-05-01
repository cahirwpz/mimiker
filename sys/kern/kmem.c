#define KL_LOG KL_KMEM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/param.h>
#include <sys/pmap.h>
#include <sys/vmem.h>
#include <sys/vm.h>
#include <sys/vm_physmem.h>
#include <sys/kasan.h>

static vmem_t *kvspace; /* Kernel virtual address space allocator. */

void kmem_bootstrap(void) {
  vmem_bootstrap();

  kvspace = vmem_create("kvspace", PAGESIZE);
  if (KERNEL_SPACE_BEGIN < (vaddr_t)__kernel_start)
    vmem_add(kvspace, KERNEL_SPACE_BEGIN,
             (vaddr_t)__kernel_start - KERNEL_SPACE_BEGIN);
  vmem_add(kvspace, (vaddr_t)vm_kernel_end,
           KERNEL_SPACE_END - (vaddr_t)vm_kernel_end);
}

static void kick_swapper(void) {
  panic("Cannot allocate more kernel memory: swapper not implemented!");
}

void *kmem_alloc(size_t size, kmem_flags_t flags) {
  assert(page_aligned_p(size));
  assert(!(flags & M_NOGROW));

  vmem_addr_t start;
  if (vmem_alloc(kvspace, size, &start, M_NOGROW))
    kick_swapper();

  /* Mark the entire block as valid */
  kasan_mark_valid((void *)start, size);

  size_t npages = size / PAGESIZE;
  vaddr_t va = start;

  while (npages > 0) {
    size_t pagecnt = 1L << log2(npages);
    vm_page_t *pg = vm_page_alloc(pagecnt);
    if (pg == NULL)
      kick_swapper();
    paddr_t pa = pg->paddr;
    for (size_t i = 0; i < pagecnt; i++)
      pmap_kenter(va + PAGESIZE * i, pa + PAGESIZE * i,
                  VM_PROT_READ | VM_PROT_WRITE);
    npages -= pagecnt;
    va += pagecnt * PAGESIZE;
  }

  if (flags & M_ZERO)
    bzero((void *)start, size);

  return (void *)start;
}

void kmem_free(void *ptr, size_t size) {
  klog("%s: free %p of size %ld", __func__, ptr, size);

  assert(page_aligned_p(ptr) && page_aligned_p(size));
  vmem_free(kvspace, (vmem_addr_t)ptr, size);

  kasan_mark_invalid((void *)ptr, size, KASAN_CODE_KMEM_USE_AFTER_FREE);

  vaddr_t va = (vaddr_t)ptr;
  vaddr_t end = va + size;
  while (va < end) {
    paddr_t pa;
    if (!pmap_extract(pmap_kernel(), va, &pa))
      panic("%s: attempted to free page that does not exist!", __func__);
    vm_page_t *pg = vm_page_find(pa);
    va += pg->size * PAGESIZE;
    vm_page_free(pg);
  }

  pmap_kremove((vaddr_t)ptr, end);
}

void *kmem_map(paddr_t pa, size_t size) {
  assert(page_aligned_p(pa) && page_aligned_p(size));

  vmem_addr_t start;
  if (vmem_alloc(kvspace, size, &start, M_NOGROW))
    kick_swapper();

  /* Mark the entire block as valid */
  kasan_mark_valid((void *)start, size);

  klog("%s: map %p of size %ld at %p", __func__, pa, size, start);

  for (size_t offset = 0; offset < size; offset += PAGESIZE)
    pmap_kenter(start + offset, pa + offset, VM_PROT_READ | VM_PROT_WRITE);

  return (void *)start;
}
