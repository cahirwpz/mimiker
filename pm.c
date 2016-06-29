#include <pm.h>
#include <malloc.h>
#include <libkern.h>
#include <mips.h>
#include <malta.h>

#define POW2(x) (1 << (x))
#define PG_FREEQ(seg, i) (&((seg)->free_queues[(i)]))

#define VM_NFREEORDER 16

TAILQ_HEAD(pg_freeq, vm_page);

typedef struct pm_seg {
  TAILQ_ENTRY(pm_seg) segq;
  pm_addr_t start;
  pm_addr_t end;
  struct pg_freeq free_queues[VM_NFREEORDER];
  vm_page_t *page_array;
} pm_seg_t;

TAILQ_HEAD(pm_seglist, pm_seg);

static struct pm_seglist seglist;
extern int _memsize;

void pm_init() {
  TAILQ_INIT(&seglist);

  pm_add_segment(MALTA_PHYS_SDRAM_BASE,
                 MALTA_PHYS_SDRAM_BASE + _memsize,
                 MIPS_KSEG0_START);
  pm_reserve(MALTA_PHYS_SDRAM_BASE,
             (pm_addr_t)kernel_sbrk_shutdown() - MIPS_KSEG0_START);
  pm_dump();
}

void pm_dump() {
  pm_seg_t *seg_it;
  vm_page_t *pg_it;

  TAILQ_FOREACH(seg_it, &seglist, segq) {
    kprintf("[pmem] segment %p - %p:\n",
            (void *)seg_it->start, (void *)seg_it->end);
    for (int i = 0; i < VM_NFREEORDER; i++) {
      if (!TAILQ_EMPTY(PG_FREEQ(seg_it, i))) {
        kprintf("[pmem]  %6dKiB:", (PAGESIZE / 1024) << i);
        TAILQ_FOREACH(pg_it, PG_FREEQ(seg_it, i), freeq)
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

  unsigned page_array_size = (end - start) / PAGESIZE;

  pm_seg_t *seg = kernel_sbrk(sizeof(pm_seg_t));
  seg->start = start;
  seg->end = end;
  seg->page_array = kernel_sbrk(page_array_size * sizeof(vm_page_t));
  TAILQ_INSERT_TAIL(&seglist, seg, segq);

  for (int i = 0; i < page_array_size; i++) {
    vm_page_t *page = &seg->page_array[i];
    page->phys_addr = seg->start + PAGESIZE * i;
    page->virt_addr = seg->start + PAGESIZE * i + vm_offset;
    page->order = 0;
    page->flags = 0;
  }

  for (int i = 0; i < VM_NFREEORDER; i++)
    TAILQ_INIT(PG_FREEQ(seg, i));

  int last_page = 0;
  int seg_size = page_array_size;

  for (int ord = VM_NFREEORDER - 1; ord >= 0; ord--) {
    while (POW2(ord) <= seg_size) {
      vm_page_t *page = &seg->page_array[last_page];
      page->flags |= VM_FREE;
      page->order = ord;
      TAILQ_INSERT_HEAD(PG_FREEQ(seg, page->order), page, freeq);
      seg_size -= POW2(ord);
      last_page += POW2(ord);
    }
  }
}

/* Takes two pages which are buddies, and merges them */
static vm_page_t *pm_merge_buddies(vm_page_t *page1, vm_page_t *page2) {
  assert(page1->order == page2->order);

  if (page1 > page2) {
    vm_page_t *tmp = page1;
    page1 = page2;
    page2 = tmp;
  }

  assert(page1 + POW2(page1->order) == page2);

  page1->order++;
  return page1;
}

static vm_page_t *pm_find_buddy(pm_seg_t *seg, vm_page_t *page) {
  vm_page_t *buddy = NULL;

  /* Page is left buddy if it's addres is divisible
   * by 2^(page->order+1), otherwise it's right buddy */

  if ((page - seg->page_array) % POW2(page->order + 1) == 0)
    buddy = page + POW2(page->order);
  else
    buddy = page - POW2(page->order);

  if (buddy->order != page->order)
    return NULL;

  /* Check if buddy is on freelist */
  vm_page_t *it;

  TAILQ_FOREACH(it, PG_FREEQ(seg, page->order), freeq)
    if (it == buddy)
      return buddy;

  return NULL;
}

static void pm_split_page(pm_seg_t *seg, vm_page_t *page) {
  int order = page->order;

  assert(!TAILQ_EMPTY(PG_FREEQ(seg, order)));

  /* Works because page is some indice of page_array */
  vm_page_t *buddy = &page[1 << (order - 1)];

  TAILQ_REMOVE(PG_FREEQ(seg, page->order), page, freeq);

  page->order = order - 1;
  buddy->order = order - 1;

  TAILQ_INSERT_HEAD(PG_FREEQ(seg, page->order), page, freeq);
  TAILQ_INSERT_HEAD(PG_FREEQ(seg, buddy->order), buddy, freeq);
  buddy->flags |= VM_FREE;
}

/* TODO this can be sped up by removing elements from list on-line. */
static void pm_reserve_from_seg(pm_seg_t *seg, pm_addr_t start, pm_addr_t end)
{
  for (int i = VM_NFREEORDER - 1; i >= 0; i--) {
    vm_page_t *pg_ptr;
    vm_page_t *pg_it = TAILQ_FIRST(PG_FREEQ(seg, i));

    while (pg_it) {
      if (PG_START(pg_it) >= start && PG_END(pg_it) <= end) {
        /* if segment is contained within (start, end) remove it from free
         * queue */
        pg_ptr = pg_it;
        while (pg_ptr < pg_it + POW2(pg_it->order)) {
          pg_ptr->flags |= VM_RESERVED;
          pg_ptr++;
        }
        TAILQ_REMOVE(PG_FREEQ(seg, pg_it->order), pg_it, freeq);
        /* Go to start of list because it's corrupted */
        pg_it = TAILQ_FIRST(PG_FREEQ(seg, i));
      } else if ((PG_START(pg_it) < start && PG_END(pg_it) > start) ||
                 (PG_START(pg_it) < end && PG_END(pg_it) > end)) {
        /* if segments intersects with (start, end) split it in half */
        pm_split_page(seg, pg_it);
        /* Go to start of list because it's corrupted */
        pg_it = TAILQ_FIRST(PG_FREEQ(seg, i));
      } else {
        pg_it = TAILQ_NEXT(pg_it, freeq);
        /* if neither of two above cases is satisfied, leave in free queue */
      }
    }
  }
}

void pm_reserve(pm_addr_t start, pm_addr_t end) {
  assert(start < end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  pm_seg_t *seg_it;

  TAILQ_FOREACH(seg_it, &seglist, segq) {
    if (seg_it->start <= start && seg_it->end >= end) {
      pm_reserve_from_seg(seg_it, start, end);
      return;
    }
  }

  panic("reserve failed (%p - %p)\n", (void *)start, (void *)end);
}

static vm_page_t *pm_alloc_from_seg(pm_seg_t *seg, size_t order) {
  vm_page_t *page = NULL;
  size_t i = order;

  /* lowest non-empty order higher than order */
  while (TAILQ_EMPTY(PG_FREEQ(seg, i)) && (i < VM_NFREEORDER))
    i++;

  if (i == VM_NFREEORDER)
    return NULL;

  while (TAILQ_EMPTY(PG_FREEQ(seg, order))) {
    pm_split_page(seg, TAILQ_FIRST(PG_FREEQ(seg, i)));
    i--;
  }

  page = TAILQ_FIRST(PG_FREEQ(seg, order));
  TAILQ_REMOVE(PG_FREEQ(seg, page->order), page, freeq);

  page->flags &= ~VM_FREE;
  return page;
}

vm_page_t *pm_alloc(size_t npages) {
  assert((npages > 0) && powerof2(npages));
  size_t order = __builtin_ctz(npages);
  pm_seg_t *seg_it;
  vm_page_t *page = NULL;
  TAILQ_FOREACH(seg_it, &seglist, segq) {
    if ((page = pm_alloc_from_seg(seg_it, order)))
      break;
  }

  return page;
}

static void pm_free_from_seg(pm_seg_t *seg, vm_page_t *page) {
  if (page->flags & VM_RESERVED)
    panic("trying to free reserved page: %p", (void *)page->phys_addr);

  if (page->flags & VM_FREE)
    panic("page is already free: %p", (void *)page->phys_addr);

  page->flags |= VM_FREE;
  TAILQ_INSERT_HEAD(PG_FREEQ(seg, page->order), page, freeq);

  vm_page_t *buddy = pm_find_buddy(seg, page);
  while (buddy != NULL) {
    TAILQ_REMOVE(PG_FREEQ(seg, page->order), page, freeq);
    TAILQ_REMOVE(PG_FREEQ(seg, buddy->order), buddy, freeq);
    page = pm_merge_buddies(page, buddy);

    TAILQ_INSERT_HEAD(PG_FREEQ(seg, page->order), page, freeq);
    page->flags |= VM_FREE;
    buddy = pm_find_buddy(seg, page);
  }
}

void pm_free(vm_page_t *page) {
  pm_seg_t *seg_it = NULL;

  TAILQ_FOREACH(seg_it, &seglist, segq) {
    if (PG_START(page) >= seg_it->start && PG_END(page) <= seg_it->end) {
      pm_free_from_seg(seg_it, page);
      return;
    }
  }

  pm_dump();
  panic("page out of range: %p", (void *)page->phys_addr);
}

vm_page_t *pm_split_alloc_page(vm_page_t *pg)
{
    assert(pg->order > 0);
    pg->order--;
    vm_page_t *res;
    res = pg + (1 << pg->order);
    res->order = pg->order;
    return res;
}

#ifdef _KERNELSPACE

/* This function hashes state of allocator. Only used to compare states
 * for testing. We cannot use string for exact comparision, because
 * this would require us to allocate some memory, which we can't do
 * at this moment. However at the moment we need to compare states only,
 * so this solution seems best. */
unsigned long pm_hash()
{
  unsigned long hash = 5381;
  pm_seg_t *seg_it;
  vm_page_t *pg_it;

  TAILQ_FOREACH(seg_it, &seglist, segq) {
    for (int i = 0; i < VM_NFREEORDER; i++) {
      if (!TAILQ_EMPTY(PG_FREEQ(seg_it, i))) {
        TAILQ_FOREACH(pg_it, PG_FREEQ(seg_it, i), freeq)
          hash = hash*33 + PG_START(pg_it);
      }
    }
  }
  return hash;
}

int main()
{
  unsigned long pre = pm_hash();

  /* Write - read test */
  vm_page_t *pg =  pm_alloc(16);
  int size = PAGESIZE*16;
  char *arr = (char*)pg->virt_addr;
  for(int i = 0; i < size; i++)
    arr[i] = 42; /* Write non-zero value */
  for(int i = 0; i < size; i++)
    assert(arr[i] == 42);

  /* Allocate deallocate test */
  const int N = 7;
  vm_page_t *pgs[N];
  for(int i = 0; i < N; i++)
    pgs[i] = pm_alloc(1 << i);
  for(int i = 0; i < N; i += 2)
    pm_free(pgs[i]);
  for(int i = 1; i < N; i += 2)
    pm_free(pgs[i]);

  assert(pre != pm_hash());
  pm_free(pg);
  assert(pre == pm_hash());

  pre = pm_hash();
  vm_page_t *pg1 = pm_alloc(4);
  vm_page_t *pg3 = pm_split_alloc_page(pg1);
  vm_page_t *pg2 = pm_split_alloc_page(pg1);
  vm_page_t *pg4 = pm_split_alloc_page(pg3);

  pm_free(pg3);
  assert(pre != pm_hash());
  pm_free(pg4);
  assert(pre != pm_hash());
  pm_free(pg2);
  assert(pre != pm_hash());
  pm_free(pg1);
  assert(pre == pm_hash());
  kprintf("Tests passed\n");
}


#endif

