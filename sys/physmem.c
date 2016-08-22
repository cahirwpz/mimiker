#include <stdc.h>
#include <malloc.h>
#include <physmem.h>

#define PM_QUEUE_OF(seg, page) ((seg)->freeq + log2((page)->size))
#define PM_FREEQ(seg, i) ((seg)->freeq + (i))

#define PM_NQUEUES 16

typedef struct pm_seg {
  TAILQ_ENTRY(pm_seg) segq;
  pm_addr_t start;
  pm_addr_t end;
  pg_list_t freeq[PM_NQUEUES];
  vm_page_t *pages;
  unsigned npages;
} pm_seg_t;

TAILQ_HEAD(pm_seglist, pm_seg);

static struct pm_seglist seglist;
extern void pm_boot();

void pm_init() {
  TAILQ_INIT(&seglist);

  pm_boot();
  pm_dump();
}

void pm_dump() {
  pm_seg_t *seg_it;
  vm_page_t *pg_it;

  TAILQ_FOREACH(seg_it, &seglist, segq) {
    kprintf("[pmem] segment %p - %p:\n",
            (void *)seg_it->start, (void *)seg_it->end);
    for (int i = 0; i < PM_NQUEUES; i++) {
      if (!TAILQ_EMPTY(PM_FREEQ(seg_it, i))) {
        kprintf("[pmem]  %6dKiB:", (PAGESIZE / 1024) << i);
        TAILQ_FOREACH(pg_it, PM_FREEQ(seg_it, i), freeq)
          kprintf(" %p", (void *)PG_START(pg_it));
        kprintf("\n");
      }
    }
  }
}

void pm_add_segment(pm_addr_t start, pm_addr_t end, vm_addr_t vm_offset) {
  assert(start < end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));
  assert(is_aligned(vm_offset, PAGESIZE));

  pm_seg_t *seg = kernel_sbrk(sizeof(pm_seg_t));
  seg->start = start;
  seg->end = end;
  seg->npages = (end - start) / PAGESIZE;
  seg->pages = kernel_sbrk(seg->npages * sizeof(vm_page_t));
  TAILQ_INSERT_TAIL(&seglist, seg, segq);

  unsigned max_size = min(PM_NQUEUES, ffs(seg->npages)) - 1;

  for (int i = 0; i < seg->npages ; i++) {
    vm_page_t *page = &seg->pages[i];
    bzero(page, sizeof(vm_page_t));
    page->paddr = seg->start + PAGESIZE * i;
    page->vaddr = seg->start + PAGESIZE * i + vm_offset;
    page->size = 1 << min(max_size, ctz(i));
  }

  for (int i = 0; i < PM_NQUEUES; i++)
    TAILQ_INIT(PM_FREEQ(seg, i));

  int curr_page = 0;
  int to_add = seg->npages;

  for (int i = PM_NQUEUES - 1; i >= 0; i--) {
    unsigned size = 1 << i;
    while (to_add >= size) {
      vm_page_t *page = &seg->pages[curr_page];
      TAILQ_INSERT_HEAD(PM_FREEQ(seg, i), page, freeq);
      page->pm_flags |= PM_MANAGED;
      to_add -= size;
      curr_page += size;
    }
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

static vm_page_t *pm_find_buddy(pm_seg_t *seg, vm_page_t *pg) {
  vm_page_t *buddy = pg;

  assert(powerof2(pg->size));

  /* When page address is divisible by (2 * size) then:
   * look at left buddy, otherwise look at right buddy */
  if ((pg - seg->pages) % (2 * pg->size) == 0)
    buddy += pg->size;
  else
    buddy -= pg->size;

  intptr_t index = buddy - seg->pages;

  if (index < 0 || index >= seg->npages)
    return NULL;

  if (buddy->size != pg->size)
    return NULL;
  
  if (!(buddy->pm_flags & PM_MANAGED))
    return NULL;

  return buddy;
}

static void pm_split_page(pm_seg_t *seg, vm_page_t *page) {
  assert(page->size > 1);
  assert(!TAILQ_EMPTY(PM_QUEUE_OF(seg, page)));

  /* It works, because every page is a member of pages! */
  unsigned size = page->size / 2;
  vm_page_t *buddy = page + size;

  assert(!(buddy->pm_flags & PM_ALLOCATED));

  TAILQ_REMOVE(PM_QUEUE_OF(seg, page), page, freeq);

  page->size = size;
  buddy->size = size;

  TAILQ_INSERT_HEAD(PM_QUEUE_OF(seg, page), page, freeq);
  TAILQ_INSERT_HEAD(PM_QUEUE_OF(seg, buddy), buddy, freeq);
  buddy->pm_flags |= PM_MANAGED;
}

/* TODO this can be sped up by removing elements from list on-line. */
static void pm_reserve_from_seg(pm_seg_t *seg, pm_addr_t start, pm_addr_t end)
{
  for (int i = PM_NQUEUES - 1; i >= 0; i--) {
    pg_list_t *queue = PM_FREEQ(seg, i);
    vm_page_t *pg = TAILQ_FIRST(queue);

    while (pg) {
      if (PG_START(pg) >= start && PG_END(pg) <= end) {
        /* if segment is contained within (start, end) remove it from free
         * queue */
        TAILQ_REMOVE(queue, pg, freeq);
        pg->pm_flags &= ~PM_MANAGED;
        int n = pg->size;
        do {
          pg->pm_flags = PM_RESERVED;
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

void pm_reserve(pm_addr_t start, pm_addr_t end) {
  assert(start < end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  kprintf("[pmem] pm_reserve: 0x%lx - 0x%lx\n", start, end);

  pm_seg_t *seg_it;

  TAILQ_FOREACH(seg_it, &seglist, segq) {
    if (seg_it->start <= start && seg_it->end >= end) {
      pm_reserve_from_seg(seg_it, start, end);
      return;
    }
  }

  panic("pm_reverse failed (%p - %p)\n", (void *)start, (void *)end);
}

static vm_page_t *pm_alloc_from_seg(pm_seg_t *seg, size_t npages) {
  size_t i, j;
  
  i = j = log2(npages);

  /* Lowest non-empty queue of size higher or equal to log2(npages). */
  while (TAILQ_EMPTY(PM_FREEQ(seg, i)) && (i < PM_NQUEUES))
    i++;

  if (i == PM_NQUEUES)
    return NULL;

  while (true) {
    vm_page_t *page = TAILQ_FIRST(PM_FREEQ(seg, i));

    if (i == j) {
      TAILQ_REMOVE(PM_FREEQ(seg, i), page, freeq);
      page->pm_flags &= ~PM_MANAGED;
      vm_page_t *pg = page;
      unsigned n = page->size;
      do {
        pg->pm_flags |= PM_ALLOCATED;
        pg++;
      } while (--n);
      return page;
    }

    pm_split_page(seg, page);
    i--;
  }
}

vm_page_t *pm_alloc(size_t npages) {
  assert((npages > 0) && powerof2(npages));

  pm_seg_t *seg_it;
  TAILQ_FOREACH(seg_it, &seglist, segq) {
    vm_page_t *page;
    if ((page = pm_alloc_from_seg(seg_it, npages))) {
      kprintf("[pmem] pm_alloc {paddr:%lx size:%ld}\n", 
              page->paddr, page->size);
      return page;
    }
  }

  return NULL;
}

static void pm_free_from_seg(pm_seg_t *seg, vm_page_t *page) {
  if (page->pm_flags & PM_RESERVED)
    panic("trying to free reserved page: %p", (void *)page->paddr);

  if (!(page->pm_flags & PM_ALLOCATED))
    panic("page is already free: %p", (void *)page->paddr);

  while (true) {
    vm_page_t *buddy = pm_find_buddy(seg, page);
    
    if (buddy == NULL) {
      TAILQ_INSERT_HEAD(PM_QUEUE_OF(seg, page), page, freeq);
      page->pm_flags |= PM_MANAGED;
      vm_page_t *pg = page;
      unsigned n = page->size;
      do {
        pg->pm_flags &= ~PM_ALLOCATED;
        pg++;
      } while (--n);
      break;
    }

    TAILQ_REMOVE(PM_QUEUE_OF(seg, buddy), buddy, freeq);
    buddy->pm_flags &= ~PM_MANAGED;
    page = pm_merge_buddies(page, buddy);
  }
}

void pm_free(vm_page_t *page) {
  pm_seg_t *seg_it = NULL;

  kprintf("[pmem] pm_free {paddr:%lx size:%ld}\n",
          page->paddr, page->size);

  TAILQ_FOREACH(seg_it, &seglist, segq) {
    if (PG_START(page) >= seg_it->start && PG_END(page) <= seg_it->end) {
      pm_free_from_seg(seg_it, page);
      return;
    }
  }

  pm_dump();
  panic("page out of range: %p", (void *)page->paddr);
}

vm_page_t *pm_split_alloc_page(vm_page_t *pg) {
  kprintf("[pmem] pm_split {paddr:%lx size:%ld}\n",
          pg->paddr, pg->size);

  assert(pg->size > 1);
  assert(pg->pm_flags & PM_ALLOCATED);

  unsigned size = pg->size / 2;
  vm_page_t *buddy = pg + size;

  pg->size = size;
  buddy->size = size;
  return buddy;
}

/* This function hashes state of allocator. Only used to compare states
 * for testing. We cannot use string for exact comparison, because
 * this would require us to allocate some memory, which we can't do
 * at this moment. However at the moment we need to compare states only,
 * so this solution seems best. */
unsigned long pm_hash() {
  unsigned long hash = 5381;
  pm_seg_t *seg_it;
  vm_page_t *pg_it;

  TAILQ_FOREACH(seg_it, &seglist, segq) {
    for (int i = 0; i < PM_NQUEUES; i++) {
      if (!TAILQ_EMPTY(PM_FREEQ(seg_it, i))) {
        TAILQ_FOREACH(pg_it, PM_FREEQ(seg_it, i), freeq)
          hash = hash * 33 + PG_START(pg_it);
      }
    }
  }
  return hash;
}
