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

static vmem_t *kvspace; /* Kernel virtual address space allocator. */
static vaddr_t maxkvaddr;

void init_kmem(void) {
  kvspace = vmem_create("kvspace", PAGESIZE);
  if (KERNEL_SPACE_BEGIN < (vaddr_t)__kernel_start)
    vmem_add(kvspace, KERNEL_SPACE_BEGIN,
             (vaddr_t)__kernel_start - KERNEL_SPACE_BEGIN);
  maxkvaddr = (vaddr_t)vm_kernel_end;
}

static void kick_swapper(void) {
  panic("Cannot allocate more kernel memory: swapper not implemented!");
}

vaddr_t kva_alloc(size_t size) {
  assert(page_aligned_p(size));
  vmem_addr_t start;

  bool restarted = false;

retry:
  if (vmem_alloc(kvspace, size, &start, M_NOGROW)) {
    if (restarted)
      return 0;

    vaddr_t old = maxkvaddr;
    maxkvaddr = pmap_growkernel(maxkvaddr + size);
    klog("%s: increase kernel end %08lx -> %08lx", __func__, old, maxkvaddr);

    if (vmem_add(kvspace, old, maxkvaddr - old))
      return 0;

    restarted = true;
    goto retry;
  }
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
  assert(page_aligned_p(size));

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
  if (va == 0)
    kick_swapper();

  kva_map(va, size, flags);

  return (void *)va;
}

vaddr_t kmem_alloc_contig(paddr_t *pap, size_t size, unsigned flags) {
  assert(page_aligned_p(size) && powerof2(size));

  size_t n = size / PAGESIZE;
  vm_page_t *pg = vm_page_alloc(n);
  if (!pg)
    return 0;

  vaddr_t va = kva_alloc(size);
  if (va == 0)
    kick_swapper();

  /* Mark the entire block as valid */
  kasan_mark_valid((void *)va, size);

  kva_map_page(va, pg->paddr, pg->size, flags);

  *pap = pg->paddr;
  return va;
}

void kmem_free(void *ptr, size_t size) {
  klog("%s: free %p of size %ld", __func__, ptr, size);
  kva_unmap((vaddr_t)ptr, size);
  vmem_free(kvspace, (vmem_addr_t)ptr, size);
}

vaddr_t kmem_map_contig(paddr_t pa, size_t size, unsigned flags) {
  assert(page_aligned_p(pa) && page_aligned_p(size));

  vaddr_t va = kva_alloc(size);
  if (va == 0)
    kick_swapper();

  /* Mark the entire block as valid */
  kasan_mark_valid((void *)va, size);

  klog("%s: map %p of size %ld at %p", __func__, pa, size, va);

  for (size_t offset = 0; offset < size; offset += PAGESIZE)
    pmap_kenter(va + offset, pa + offset, VM_PROT_READ | VM_PROT_WRITE, flags);

  return va;
}
