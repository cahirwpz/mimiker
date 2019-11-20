#define KL_LOG KL_PHYSMEM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/kbss.h>
#include <sys/libkern.h>
#include <sys/mutex.h>
#include <sys/vm_physmem.h>

#define PG_FREELIST(page) (&freelist[log2((page)->size)])

#define PM_NQUEUES 16U

typedef struct vm_physseg {
  TAILQ_ENTRY(vm_physseg) seglink;
  paddr_t start;
  paddr_t end;
  unsigned npages;
  vm_page_t *pages;
} vm_physseg_t;

static TAILQ_HEAD(, vm_physseg) seglist = TAILQ_HEAD_INITIALIZER(seglist);
static vm_pagelist_t freelist[PM_NQUEUES];
static mtx_t *physmem_lock = &MTX_INITIALIZER(0);

vm_physseg_t *vm_physseg_alloc(paddr_t start, paddr_t end) {
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);

  static vm_physseg_t freeseg[VM_PHYSSEG_NMAX];
  static unsigned freeseg_last = 0;

  SCOPED_MTX_LOCK(physmem_lock);

  assert(freeseg_last < VM_PHYSSEG_NMAX - 1);

  vm_physseg_t *seg = &freeseg[freeseg_last++];

  seg->start = start;
  seg->end = end;
  seg->npages = (end - start) / PAGESIZE;

  TAILQ_INSERT_TAIL(&seglist, seg, seglink);

  return seg;
}

char *__kernel_end;

void vm_page_init(void) {
  vm_physseg_t *seg;

  for (unsigned i = 0; i < PM_NQUEUES; i++)
    TAILQ_INIT(&freelist[i]);

  /* Allocate contiguous array of vm_page_t to cover all physical memory. */
  size_t npages = 0;
  TAILQ_FOREACH (seg, &seglist, seglink)
    npages += seg->npages;

  vm_page_t *pages = kbss_grow(npages * sizeof(vm_page_t));
  bzero(pages, npages * sizeof(vm_page_t));

  /*
   * Let's fix size of kernel bss section. We need to tell physical memory
   * allocator not to manage memory used by the kernel image along with all
   * memory allocated using \a kbss_grow.
   */
  __kernel_end = kbss_fix();

  TAILQ_FOREACH (seg, &seglist, seglink) {
    size_t max_size = min(PM_NQUEUES, ffs(seg->npages)) - 1;

    for (unsigned i = 0; i < seg->npages; i++) {
      pages[i].paddr = seg->start + PAGESIZE * i;
      pages[i].size = 1 << min(max_size, ctz(i));
    }

    int curr_page = 0;
    unsigned to_add = seg->npages;

    for (int i = PM_NQUEUES - 1; i >= 0; i--) {
      unsigned size = 1 << i;
      while (to_add >= size) {
        vm_page_t *page = &pages[curr_page];
        TAILQ_INSERT_HEAD(&freelist[i], page, freeq);
        page->flags |= PG_MANAGED;
        to_add -= size;
        curr_page += size;
      }
    }

    seg->pages = pages;
    pages += seg->npages;
  }
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

static void pm_split_page(vm_physseg_t *seg, vm_page_t *page) {
  assert(page->size > 1);
  assert(!TAILQ_EMPTY(PG_FREELIST(page)));

  /* It works, because every page is a member of pages! */
  unsigned size = page->size / 2;
  vm_page_t *buddy = page + size;

  assert(!(buddy->flags & PG_ALLOCATED));

  TAILQ_REMOVE(PG_FREELIST(page), page, freeq);

  page->size = size;
  buddy->size = size;

  TAILQ_INSERT_HEAD(PG_FREELIST(page), page, freeq);
  TAILQ_INSERT_HEAD(PG_FREELIST(buddy), buddy, freeq);
  buddy->flags |= PG_MANAGED;
}

/* TODO this can be sped up by removing elements from list on-line. */
void vm_physseg_reserve(vm_physseg_t *seg, paddr_t start, paddr_t end) {
  assert(page_aligned_p(start) && page_aligned_p(end) && start < end);
  assert(seg->start <= start && end <= seg->end);

  klog("pm_seg_reserve: %08lx - %08lx from [%08lx, %08lx]", start, end,
       seg->start, seg->end);

  for (int i = PM_NQUEUES - 1; i >= 0; i--) {
    vm_pagelist_t *queue = &freelist[i];
    vm_page_t *pg = TAILQ_FIRST(queue);

    while (pg) {
      if (PG_START(pg) >= start && PG_END(pg) <= end) {
        /* if segment is contained within (start, end) remove it from free
         * queue */
        TAILQ_REMOVE(queue, pg, freeq);
        pg->flags &= ~PG_MANAGED;
        int n = pg->size;
        do {
          pg->flags = PG_RESERVED;
          pg++;
        } while (--n);
        /* List has been changed so start over! */
        pg = TAILQ_FIRST(queue);
      } else if ((PG_START(pg) < start && PG_END(pg) > start) ||
                 (PG_START(pg) < end && PG_END(pg) > end)) {
        /* if segments intersects with (start, end) split it in half */
        pm_split_page(seg, pg);
        /* List has been changed so start over! */
        pg = TAILQ_FIRST(queue);
      } else {
        pg = TAILQ_NEXT(pg, freeq);
        /* if neither of two above cases is satisfied, leave in free queue */
      }
    }
  }
}

static vm_page_t *pm_alloc_from_seg(vm_physseg_t *seg, size_t npages) {
  size_t i, j;

  i = j = log2(npages);

  /* Lowest non-empty queue of size higher or equal to log2(npages). */
  while (TAILQ_EMPTY(&freelist[i]) && (i < PM_NQUEUES))
    i++;

  if (i == PM_NQUEUES)
    return NULL;

  while (true) {
    vm_page_t *page = TAILQ_FIRST(&freelist[i]);

    if (i == j) {
      TAILQ_REMOVE(&freelist[i], page, freeq);
      page->flags &= ~PG_MANAGED;
      vm_page_t *pg = page;
      unsigned n = page->size;
      do {
        pg->flags |= PG_ALLOCATED;
        pg->flags &= ~(PG_REFERENCED | PG_MODIFIED);
        pg++;
      } while (--n);
      return page;
    }

    pm_split_page(seg, page);
    i--;
  }
}

vm_page_t *vm_page_alloc(size_t npages) {
  assert((npages > 0) && powerof2(npages));

  SCOPED_MTX_LOCK(physmem_lock);

  vm_physseg_t *seg_it;
  TAILQ_FOREACH (seg_it, &seglist, seglink) {
    vm_page_t *page;
    if ((page = pm_alloc_from_seg(seg_it, npages))) {
      klog("pm_alloc {paddr:%lx size:%ld}", page->paddr, page->size);
      return page;
    }
  }

  return NULL;
}

static void pm_free_from_seg(vm_physseg_t *seg, vm_page_t *page) {
  if (page->flags & PG_RESERVED)
    panic("trying to free reserved page: %p", (void *)page->paddr);

  if (!(page->flags & PG_ALLOCATED))
    panic("page is already free: %p", (void *)page->paddr);

  while (true) {
    vm_page_t *buddy = pm_find_buddy(seg, page);

    if (buddy == NULL) {
      TAILQ_INSERT_HEAD(PG_FREELIST(page), page, freeq);
      page->flags |= PG_MANAGED;
      vm_page_t *pg = page;
      unsigned n = page->size;
      do {
        pg->flags &= ~PG_ALLOCATED;
        pg++;
      } while (--n);
      break;
    }

    TAILQ_REMOVE(PG_FREELIST(buddy), buddy, freeq);
    buddy->flags &= ~PG_MANAGED;
    page = pm_merge_buddies(page, buddy);
  }
}

void vm_page_free(vm_page_t *page) {
  vm_physseg_t *seg_it = NULL;

  klog("pm_free {paddr:%lx size:%ld}", page->paddr, page->size);

  SCOPED_MTX_LOCK(physmem_lock);

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
