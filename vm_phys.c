#include <vm_phys.h>
#include <queue.h>
#include <malloc.h>
#include <libkern.h>

#define POW2(x) (1 << (x))
#define PG_START(pg) ((pg)->phys_addr)
#define PG_END(pg) ((pg)->phys_addr + (1 << (pg->order)) * PAGESIZE)
#define PG_FREEQ(seg, i) (&((seg)->free_queues[(i)]))

TAILQ_HEAD(vm_seglist, vm_phys_seg);

static struct vm_seglist seglist;

void vm_phys_print_free_pages() {
  vm_phys_seg_t *seg_it;
  TAILQ_FOREACH(seg_it, &seglist, segq) {
    kprintf("[buddy] segment %p - %p:\n", seg_it->start, seg_it->end);
    for (int i = 0; i < VM_NFREEORDER; i++) {
      if (!TAILQ_EMPTY(PG_FREEQ(seg_it, i))) {
        kprintf("[buddy]  %6dKiB:", (PAGESIZE / 1024) << i);
        vm_page_t *pg_it;
        TAILQ_FOREACH(pg_it, PG_FREEQ(seg_it, i), freeq)
          kprintf(" %p", PG_START(pg_it));
        kprintf("\n");
      }
    }
  }
}

void vm_phys_add_seg(vm_paddr_t start, vm_paddr_t end, vm_paddr_t vm_offset) {
  struct vm_phys_seg *seg = kernel_sbrk(sizeof(struct vm_phys_seg));
  int page_array_size = (end - start) / PAGESIZE;

  seg->start = start;
  seg->end = end;
  seg->page_array = kernel_sbrk(page_array_size);
  TAILQ_INSERT_TAIL(&seglist, seg, segq);

  if ((vm_paddr_t)ALIGN(seg->start, PAGESIZE) != (vm_paddr_t) seg->start)
    panic("start not page aligned");
  if ((vm_paddr_t)ALIGN(seg->end, PAGESIZE) != (vm_paddr_t) seg->end)
    panic("end not page aligned");

  for (int i = 0; i < page_array_size; i++) {
    seg->page_array[i].phys_addr = seg->start + PAGESIZE * i;
    seg->page_array[i].virt_addr = seg->start + PAGESIZE * i + vm_offset;
    seg->page_array[i].order = 0;
    seg->page_array[i].flags = 0;
  }

  for (int i = 0; i < VM_NFREEORDER; i++)
    TAILQ_INIT(PG_FREEQ(seg, i));
  int last_page = 0;
  int seg_size = page_array_size;
  for (int ord = VM_NFREEORDER - 1; ord >= 0; ord--)
    while (POW2(ord) <= seg_size) {
      vm_page_t *page = &seg->page_array[last_page];
      page->flags |= VM_FREE;
      page->order = ord;
      TAILQ_INSERT_HEAD(PG_FREEQ(seg, page->order), page, freeq);
      seg_size -= POW2(ord);
      last_page += POW2(ord);
    }
}

void vm_phys_init() {
  TAILQ_INIT(&seglist);
}

/* Takes two pages which are buddies, and merges them */
static vm_page_t *merge(vm_page_t *page1, vm_page_t *page2) {
  if (page1->order != page2->order)
    panic("trying to connect pages of different orders");
  if (page1 > page2) {
    vm_page_t *tmp = page1;
    page1 = page2;
    page2 = tmp;
  }
  if (page1 + POW2(page1->order) != page2)
    panic("pages are not buddies");
  page1->order++;
  return page1;
}

static vm_page_t *find_buddy(vm_phys_seg_t *seg, vm_page_t *page) {
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
      break;

  return buddy;
}

static void split_page(vm_phys_seg_t *seg, vm_page_t *page) {
  int order = page->order;

  if (TAILQ_EMPTY(PG_FREEQ(seg, order)))
    panic("cannot split segment");

  /* Works because page is some indice of page_array */
  vm_page_t *buddy = &page[1 << (order - 1)];

  TAILQ_REMOVE(PG_FREEQ(seg, page->order), page, freeq);

  page->order = order - 1;
  buddy->order = order - 1;

  TAILQ_INSERT_TAIL(PG_FREEQ(seg, page->order), page, freeq);
  TAILQ_INSERT_TAIL(PG_FREEQ(seg, buddy->order), buddy, freeq);
  buddy->flags |= VM_FREE;
}

static void vm_phys_reserve_from_seg(vm_phys_seg_t *seg, vm_paddr_t start,
                                     vm_paddr_t end) {
  for (int i = VM_NFREEORDER - 1; i >= 0; i--) {
    vm_page_t *pg_ptr;
    vm_page_t *pg_it = TAILQ_FIRST(PG_FREEQ(seg, i));
    while (pg_it) {
      if (PG_START(pg_it) >= start && PG_END(pg_it) <= end) {
        /* if segment is containted within (start, end) remove it from free
         * queue */
        pg_ptr = pg_it;
        while (pg_ptr++ < pg_it + POW2(pg_it->order))
          pg_ptr->flags |= VM_RESERVED;
        TAILQ_REMOVE(PG_FREEQ(seg, pg_it->order), pg_it, freeq);
        /* Go to start of list because it's corrupted */
        pg_it = TAILQ_FIRST(PG_FREEQ(seg, i));
      } else if ((PG_START(pg_it) < start && PG_END(pg_it) > start) ||
                 (PG_START(pg_it) < end && PG_END(pg_it) > end)) {
        /* if segments intersects with (start, end) split it in half */
        split_page(seg, pg_it);
        /* Go to start of list because it's corrupted */
        pg_it = TAILQ_FIRST(PG_FREEQ(seg, i));
      } else {
        pg_it = TAILQ_NEXT(pg_it, freeq);
        /* if neither of two above cases is satisfied, leave in free queue */
      }
    }
  }
}

void vm_phys_reserve(vm_paddr_t start, vm_paddr_t end) {
  vm_phys_seg_t *seg_it;

  if ((vm_paddr_t)ALIGN(start, PAGESIZE) != start)
    panic("start not page aligned");
  if ((vm_paddr_t)ALIGN(end, PAGESIZE) != end)
    panic("end not page aligned");

  TAILQ_FOREACH(seg_it, &seglist, segq) {
    if (seg_it->start <= start && seg_it->end >= end) {
      vm_phys_reserve_from_seg(seg_it, start, end);
      break;
    }
  }
}

static vm_page_t *vm_phys_alloc_from_seg(vm_phys_seg_t *seg, size_t order) {
  vm_page_t *page = NULL;
  size_t i = order;

  /* lowest non-empty order higer than order */
  while (TAILQ_EMPTY(PG_FREEQ(seg, i)) && (i < VM_NFREEORDER))
    i++;

  if (i == VM_NFREEORDER)
    return NULL;

  while (TAILQ_EMPTY(PG_FREEQ(seg, order))) {
    split_page(seg, TAILQ_FIRST(PG_FREEQ(seg, i)));
    i--;
  }

  page = TAILQ_FIRST(PG_FREEQ(seg, order));
  TAILQ_REMOVE(PG_FREEQ(seg, page->order), page, freeq);

  page->flags &= ~VM_FREE;
  return page;
}

vm_page_t *vm_phys_alloc(size_t order) {
  vm_phys_seg_t *seg_it;
  vm_page_t *page = NULL;
  TAILQ_FOREACH(seg_it, &seglist, segq) {
    page = vm_phys_alloc_from_seg(seg_it, order);
    if (page) break;
  }

  return page;
}

static void vm_phys_free_from_seg(vm_phys_seg_t *seg, vm_page_t *page) {
  if (page->flags & VM_RESERVED)
    panic("trying to free reserved page: %p", page->phys_addr);

  if (page->flags & VM_FREE)
    panic("page is already free: %p", page->phys_addr);

  page->flags |= VM_FREE;
  TAILQ_INSERT_HEAD(PG_FREEQ(seg, page->order), page, freeq);

  vm_page_t *buddy = find_buddy(seg, page);
  while (buddy != NULL) {
    TAILQ_REMOVE(PG_FREEQ(seg, page->order), page, freeq);
    TAILQ_REMOVE(PG_FREEQ(seg, buddy->order), buddy, freeq);
    page = merge(page, buddy);

    TAILQ_INSERT_HEAD(PG_FREEQ(seg, page->order), page, freeq);
    page->flags |= VM_FREE;
    buddy = find_buddy(seg, page);
  }
}

void vm_phys_free(vm_page_t *page) {
  vm_phys_seg_t *seg_it = NULL;

  TAILQ_FOREACH(seg_it, &seglist, segq) {
    if (PG_START(page) >= seg_it->start && PG_END(page) <= seg_it->end) {
      vm_phys_free_from_seg(seg_it, page);
      return;
    }
  }

  vm_phys_print_free_pages();
  panic("page out of range: %p", page->phys_addr);
}
