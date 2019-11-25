#define KL_LOG KL_KMEM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/param.h>
#include <sys/pmap.h>
#include <sys/vmem.h>
#include <sys/vm.h>
#include <sys/vm_physmem.h>

static vmem_t *kvspace; /* Kernel virtual address space allocator. */

void kmem_bootstrap(void) {
  kvspace = vmem_create("kvspace", 0, 0, PAGESIZE);
  if (KERNEL_SPACE_BEGIN < (vaddr_t)__kernel_start)
    vmem_add(kvspace, KERNEL_SPACE_BEGIN,
             (vaddr_t)__kernel_start - KERNEL_SPACE_BEGIN);
  vmem_add(kvspace, (vaddr_t)vm_kernel_end,
           KERNEL_SPACE_END - (vaddr_t)vm_kernel_end);
}

void *kmem_alloc(size_t size, unsigned flags) {
  assert(page_aligned_p(size));

  vmem_addr_t start;
  if (vmem_alloc(kvspace, size, &start))
    goto noswap;

  size_t npages = size / PAGESIZE;
  vaddr_t va = start;

  while (npages > 0) {
    size_t pagecnt = 1L << log2(npages);
    vm_page_t *pg = vm_page_alloc(pagecnt);
    if (pg == NULL)
      goto noswap;
    paddr_t pa = pg->paddr;
    for (size_t i = 0; i < pagecnt; i++)
      pmap_kenter(va + PAGESIZE * i, pa + PAGESIZE * i,
                  VM_PROT_READ | VM_PROT_WRITE);
    npages -= pagecnt;
    va += pagecnt * PAGESIZE;
  }

  return (void *)start;

noswap:
  panic("Cannot allocate more kernel memory: swapper not implemented!");
}

void kmem_free(void *ptr) {
  /* TODO: not implemented */
}
