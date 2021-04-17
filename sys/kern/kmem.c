#define KL_LOG KL_KMEM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/param.h>
#include <sys/pmap.h>
#include <sys/kmem.h>
#include <sys/vmem.h>
#include <sys/vm.h>
#include <sys/vm_physmem.h>
#include <sys/kasan.h>
#include <sys/mutex.h>

static vmem_t *kvspace; /* Kernel virtual address space allocator. */

void init_kmem(void) {
  kvspace = vmem_create("kvspace", PAGESIZE);
  if (KERNEL_SPACE_BEGIN < (vaddr_t)__kernel_start)
    vmem_add(kvspace, KERNEL_SPACE_BEGIN,
             (vaddr_t)__kernel_start - KERNEL_SPACE_BEGIN);
}

static void kick_swapper(void) {
  panic("Cannot allocate more kernel memory: swapper not implemented!");
}

vaddr_t kva_alloc(size_t size) {
  assert(page_aligned_p(size));
  vmem_addr_t start;
  vaddr_t old = atomic_load(&vm_kernel_end);

  /*
   * Let's assume that vmem_alloc failed.
   * Then we increase active virtual address space for kernel and add new
   * addresses into kvspace. But there is a time window between vmem_add and
   * vmem_alloc where other thread can call kva_alloc and steal memory prepared
   * by us for vmem. In that scenario it's possible that second call to
   * vmem_alloc fails so we need to repeat pmap_growkernel and restart
   * vmem_alloc. We do that until success because kva_alloc should never failed.
   */
  while (vmem_alloc(kvspace, size, &start, M_NOGROW)) {
    mtx_lock(&vm_kernel_end_lock);
    /* Check if other thread called pmap_growkernel between vmem_alloc and
     * mtx_lock. */
    if (vm_kernel_end > old) {
      old = vm_kernel_end;
      mtx_unlock(&vm_kernel_end_lock);
      continue;
    }

    pmap_growkernel(old + size);
    klog("%s: increase kernel end %08lx -> %08lx", __func__, old,
         vm_kernel_end);

    int error = vmem_add(kvspace, old, vm_kernel_end - old);
    assert(error == 0);
    mtx_unlock(&vm_kernel_end_lock);
  }

  assert(start != 0);
  return start;
}

void kva_free(vaddr_t ptr, size_t size) {
  assert(page_aligned_p(ptr) && page_aligned_p(size));
  vmem_free(kvspace, ptr, size);
}

static void kva_map_page(vaddr_t va, paddr_t pa, size_t n, unsigned flags) {
  for (size_t i = 0; i < n; i++, va += PAGESIZE, pa += PAGESIZE)
    pmap_kenter(va, pa, VM_PROT_READ | VM_PROT_WRITE, flags);
}

void kva_map(vaddr_t ptr, size_t size, kmem_flags_t flags) {
  assert(page_aligned_p(ptr) && page_aligned_p(size));

  /* Mark the entire block as valid */
  kasan_mark_valid((void *)ptr, size);

  size_t npages = size / PAGESIZE;

  vm_pagelist_t pglist;
  int error = vm_pagelist_alloc(npages, &pglist);
  if (error)
    kick_swapper();

  vaddr_t va = ptr;
  vm_page_t *pg;
  TAILQ_FOREACH (pg, &pglist, pageq) {
    kva_map_page(va, pg->paddr, pg->size, 0);
    va += pg->size * PAGESIZE;
  }

  if (flags & M_ZERO)
    bzero((void *)ptr, size);
}

vm_page_t *kva_find_page(vaddr_t ptr) {
  paddr_t pa;
  if (pmap_extract(pmap_kernel(), ptr, &pa))
    return vm_page_find(pa);
  return NULL;
}

void kva_unmap(vaddr_t ptr, size_t size) {
  assert(page_aligned_p(ptr) && page_aligned_p(size));

  kasan_mark_invalid((void *)ptr, size, KASAN_CODE_KMEM_FREED);

  vm_pagelist_t pglist;
  TAILQ_INIT(&pglist);

  vaddr_t va = ptr;
  while (va < ptr + size) {
    vm_page_t *pg = kva_find_page(va);
    assert(pg != NULL);
    va += pg->size * PAGESIZE;
    TAILQ_INSERT_TAIL(&pglist, pg, pageq);
  }

  pmap_kremove(ptr, size);
  vm_pagelist_free(&pglist);
}

void *kmem_alloc(size_t size, kmem_flags_t flags) {
  assert(page_aligned_p(size));
  assert(!(flags & M_NOGROW));

  vaddr_t va = kva_alloc(size);
  kva_map(va, size, flags);

  return (void *)va;
}

vaddr_t kmem_alloc_contig(paddr_t *pap, size_t size, unsigned flags) {
  assert(page_aligned_p(size) && powerof2(size));

  size_t n = size / PAGESIZE;
  vm_page_t *pg = vm_page_alloc(n);
  if (!pg)
    return 0;

  if (pap)
    *pap = pg->paddr;
  
  return kmem_map_contig(pg->paddr, pg->size, flags);
}

void kmem_free(void *ptr, size_t size) {
  klog("%s: free %p of size %ld", __func__, ptr, size);
  kva_unmap((vaddr_t)ptr, size);
  vmem_free(kvspace, (vmem_addr_t)ptr, size);
}

vaddr_t kmem_map_contig(paddr_t pa, size_t size, unsigned flags) {
  assert(page_aligned_p(pa) && page_aligned_p(size));

  vaddr_t va = kva_alloc(size);

  /* Mark the entire block as valid */
  kasan_mark_valid((void *)va, size);

  klog("%s: map %p of size %ld at %p", __func__, pa, size, va);

  kva_map_page(va, pa, size / PAGESIZE, flags);

  return va;
}
