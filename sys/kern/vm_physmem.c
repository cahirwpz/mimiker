#define KL_LOG KL_PHYSMEM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/mutex.h>
#include <sys/pmap.h>
#include <sys/vm_physmem.h>

#define FREELIST(page) (&freelist[log2((page)->size)])
#define PAGECOUNT(page) (pagecount[log2((page)->size)])

#define PM_NQUEUES 16U

typedef struct vm_physseg {
  TAILQ_ENTRY(vm_physseg) seglink;
  paddr_t start;
  paddr_t end;
  size_t npages;
  bool used; /* all memory in this segment must be marked as used */
  vm_page_t *pages;
} vm_physseg_t;

static TAILQ_HEAD(, vm_physseg) seglist = TAILQ_HEAD_INITIALIZER(seglist);
static vm_pagelist_t freelist[PM_NQUEUES];
static size_t pagecount[PM_NQUEUES];
static mtx_t *physmem_lock = &MTX_INITIALIZER(0);

void _vm_physseg_plug(paddr_t start, paddr_t end, bool used) {
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);

  static vm_physseg_t freeseg[VM_PHYSSEG_NMAX];
  static unsigned freeseg_last = 0;

  SCOPED_MTX_LOCK(physmem_lock);

  assert(freeseg_last < VM_PHYSSEG_NMAX - 1);

  vm_physseg_t *seg = &freeseg[freeseg_last++];

  seg->start = start;
  seg->end = end;
  seg->npages = (end - start) / PAGESIZE;
  seg->used = used;

  TAILQ_INSERT_TAIL(&seglist, seg, seglink);
}

static bool vm_boot_done = false;
void *vm_kernel_end = __kernel_end;

static void *vm_boot_alloc(size_t n) {
  assert(!vm_boot_done);

  void *begin = align(vm_kernel_end, sizeof(long));
  void *end = align(begin + n, PAGESIZE);

  vm_physseg_t *seg = TAILQ_FIRST(&seglist);

  while (seg && seg->used)
    seg = TAILQ_NEXT(seg, seglink);

  assert(seg != NULL);

  for (void *va = align(begin, PAGESIZE); va < end; va += PAGESIZE) {
    paddr_t pa = seg->start;
    seg->start += PAGESIZE;
    if (--seg->npages == 0) {
      TAILQ_REMOVE(&seglist, seg, seglink);
      seg = TAILQ_FIRST(&seglist);
    }

    pmap_kenter((vaddr_t)va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
  }

  vm_kernel_end += n;

  return begin;
}

static void vm_boot_finish(void) {
  vm_kernel_end = align(vm_kernel_end, PAGESIZE);
  vm_boot_done = true;
}

void init_vm_page(void) {
  vm_physseg_t *seg;

  for (unsigned i = 0; i < PM_NQUEUES; i++)
    TAILQ_INIT(&freelist[i]);

  /* Allocate contiguous array of vm_page_t to cover all physical memory. */
  size_t npages = 0;
  TAILQ_FOREACH (seg, &seglist, seglink)
    npages += seg->npages;

  vm_page_t *pages = vm_boot_alloc(npages * sizeof(vm_page_t));
  bzero(pages, npages * sizeof(vm_page_t));

  TAILQ_FOREACH (seg, &seglist, seglink) {
    /* Configure all pages in the segment. */
    for (unsigned i = 0; i < seg->npages; i++) {
      vm_page_t *page = &pages[i];
      paddr_t pa = seg->start + i * PAGESIZE;
      unsigned size = 1 << min(PM_NQUEUES - 1, ctz(pa / PAGESIZE));
      if (pa + size * PAGESIZE > seg->end)
        size = 1 << min(PM_NQUEUES - 1, log2((seg->end ^ pa) / PAGESIZE));
      page->paddr = pa;
      page->size = size;
      page->flags = seg->used ? PG_ALLOCATED : 0;
    }

    /* Insert pages into free lists of corresponding size. */
    if (!seg->used) {
      for (unsigned i = 0; i < seg->npages;) {
        vm_page_t *page = &pages[i];
        TAILQ_INSERT_TAIL(FREELIST(page), page, freeq);
        PAGECOUNT(page)++;
        page->flags |= PG_MANAGED;
        i += page->size;
      }
    }

    seg->pages = pages;
    pages += seg->npages;
  }

  vm_boot_finish();
}

/* Takes two pages which are buddies, and merges them */
static vm_page_t *pm_merge_buddies(vm_page_t *pg1, vm_page_t *pg2) {
  assert(pg1->size == pg2->size);

  if (pg1 > pg2)
    swap(pg1, pg2);

  assert(pg1 + pg1->size == pg2);

  pg1->size *= 2;
  return pg1;
}

static vm_page_t *pm_find_buddy(vm_physseg_t *seg, vm_page_t *pg) {
  vm_page_t *buddy = pg;

  assert(powerof2(pg->size));

  /* When page address is divisible by (2 * size) then:
   * look at left buddy, otherwise look at right buddy */
  if ((pg - seg->pages) % (2 * pg->size) == 0)
    buddy += pg->size;
  else
    buddy -= pg->size;

  intptr_t index = buddy - seg->pages;

  if (index < 0 || index >= (intptr_t)seg->npages)
    return NULL;

  if (buddy->size != pg->size)
    return NULL;

  if (!(buddy->flags & PG_MANAGED))
    return NULL;

  return buddy;
}

static void pm_split_page(vm_page_t *page) {
  assert(page->size > 1);
  assert(!TAILQ_EMPTY(FREELIST(page)));

  /* It works, because every page is a member of pages! */
  unsigned size = page->size / 2;
  vm_page_t *buddy = page + size;

  assert(!(buddy->flags & PG_ALLOCATED));

  TAILQ_REMOVE(FREELIST(page), page, freeq);
  PAGECOUNT(page)--;

  page->size = size;
  buddy->size = size;

  TAILQ_INSERT_HEAD(FREELIST(page), page, freeq);
  PAGECOUNT(page)++;
  TAILQ_INSERT_HEAD(FREELIST(buddy), buddy, freeq);
  PAGECOUNT(buddy)++;
  buddy->flags |= PG_MANAGED;
}

vm_page_t *vm_page_alloc(size_t npages) {
  assert((npages > 0) && powerof2(npages));

  SCOPED_MTX_LOCK(physmem_lock);

  size_t n = log2(npages);
  size_t i = n;

  /* Lowest non-empty queue of size higher or equal to log2(npages). */
  while (i < PM_NQUEUES && TAILQ_EMPTY(&freelist[i]))
    i++;

  if (i == PM_NQUEUES)
    return NULL;

  for (; i > n; i--)
    pm_split_page(TAILQ_FIRST(&freelist[i]));

  vm_page_t *page = TAILQ_FIRST(&freelist[i]);
  klog("%s: allocated %lx of size %ld", __func__, page->paddr, page->size);
  TAILQ_REMOVE(&freelist[i], page, freeq);
  pagecount[i]--;
  page->flags &= ~PG_MANAGED;
  for (unsigned j = 0; j < page->size; j++) {
    page[j].flags |= PG_ALLOCATED;
    page[j].flags &= ~(PG_REFERENCED | PG_MODIFIED);
  }

  return page;
}

static void pm_free_from_seg(vm_physseg_t *seg, vm_page_t *page) {
  if (!(page->flags & PG_ALLOCATED))
    panic("page is already free: %p", (void *)page->paddr);

  vm_page_t *buddy;
  while ((buddy = pm_find_buddy(seg, page))) {
    TAILQ_REMOVE(FREELIST(buddy), buddy, freeq);
    PAGECOUNT(buddy)--;
    buddy->flags &= ~PG_MANAGED;
    page = pm_merge_buddies(page, buddy);
  }

  TAILQ_INSERT_HEAD(FREELIST(page), page, freeq);
  PAGECOUNT(page)++;
  page->flags |= PG_MANAGED;
  for (unsigned i = 0; i < page->size; i++)
    page[i].flags &= ~PG_ALLOCATED;
}

void vm_page_free(vm_page_t *page) {
  vm_physseg_t *seg_it = NULL;

  SCOPED_MTX_LOCK(physmem_lock);

  klog("%s: free %lx of size %ld", __func__, page->paddr, page->size);

  TAILQ_FOREACH (seg_it, &seglist, seglink) {
    if (PG_START(page) >= seg_it->start && PG_END(page) <= seg_it->end) {
      pm_free_from_seg(seg_it, page);
      return;
    }
  }

  panic("page out of range: %p", (void *)page->paddr);
}

vm_page_t *vm_page_find(paddr_t pa) {
  SCOPED_MTX_LOCK(physmem_lock);

  vm_physseg_t *seg_it;
  TAILQ_FOREACH (seg_it, &seglist, seglink) {
    if (seg_it->start <= pa && pa < seg_it->end) {
      intptr_t index = (pa - seg_it->start) / PAGESIZE;
      return &seg_it->pages[index];
    }
  }

  return NULL;
}
