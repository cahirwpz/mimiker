#include <tlb.h>
#include <vm_map.h>
#include <malloc.h>
#include <libkern.h>
#include <mips.h>
#include <pmap.h>
#include <tree.h>
#include <pager.h>

static vm_map_t *active_vm_map;

static inline int vm_page_cmp(vm_page_t *a, vm_page_t *b) {
  if (a->vm_offset < b->vm_offset)
    return -1;
  return a->vm_offset - b->vm_offset;
}

static inline int vm_map_entry_cmp(vm_map_entry_t *a, vm_map_entry_t *b) {
  if (a->start < b->start)
    return -1;
  return a->start - b->start;
}

RB_PROTOTYPE_STATIC(vm_object_tree, vm_page, obj.tree, vm_page_cmp);
RB_GENERATE(vm_object_tree, vm_page, obj.tree, vm_page_cmp);

SPLAY_PROTOTYPE(vm_map_tree, vm_map_entry, map_tree, vm_map_entry_cmp);
SPLAY_GENERATE(vm_map_tree, vm_map_entry, map_tree, vm_map_entry_cmp);

static MALLOC_DEFINE(mpool, "inital vm_map memory pool");

static vm_object_t *allocate_object() {
  vm_object_t *obj = kmalloc(mpool, sizeof(vm_object_t), 0);
  obj->pgr = pager_allocate(DEFAULT_PAGER);
  TAILQ_INIT(&obj->list);
  RB_INIT(&obj->tree);
  return obj;
}

static vm_map_entry_t *allocate_entry() {
  vm_map_entry_t *entry = kmalloc(mpool, sizeof(vm_map_entry_t), 0);
  entry->object = allocate_object();
  return entry;
}

static bool insert_entry(vm_map_t *vm_map, vm_map_entry_t *entry) {
  if (!SPLAY_INSERT(vm_map_tree, &vm_map->tree, entry)) {
    vm_map_entry_t *next = SPLAY_NEXT(vm_map_tree, &vm_map->tree, entry);
    if (next)
      TAILQ_INSERT_BEFORE(next, entry, map_list);
    else
      TAILQ_INSERT_TAIL(&vm_map->list, entry, map_list);
    vm_map->nentries++;
    return true;
  } else
    return false;
}

void vm_map_init() {
  vm_page_t *pg = pm_alloc(4);
  kmalloc_init(mpool);
  kmalloc_add_arena(mpool, pg->vaddr, PG_SIZE(pg));
  active_vm_map = NULL;
}

typedef struct {
  vm_map_type_t type;
  vaddr_t start, end;
} vm_map_range_t;

static vm_map_range_t vm_map_range[] = {
  {KERNEL_VM_MAP, 0xc0400000, 0xe0000000},
  {USER_VM_MAP, 0x00000000, 0x80000000},
  {0},
};

vm_map_t *vm_map_new(vm_map_type_t type, asid_t asid) {
  vm_map_t *vm_map = kmalloc(mpool, sizeof(vm_map_t), 0);
  TAILQ_INIT(&vm_map->list);
  SPLAY_INIT(&vm_map->tree);
  pmap_init(&vm_map->pmap);
  vm_map->nentries = 0;
  vm_map->pmap.asid = asid;

  vm_map_range_t *range;

  for (range = vm_map_range; range->type; range++)
    if (type == range->type)
      break;

  if (!range->type)
    panic("[vm_map] Address range %d unknown!\n", type);

  vm_map->start = range->start;
  vm_map->end = range->end;
  return vm_map;
}

static void vm_object_free(vm_object_t *obj) {
  pager_delete(obj->pgr);
  while (!TAILQ_EMPTY(&obj->list)) {
    vm_page_t *pg = TAILQ_FIRST(&obj->list);
    TAILQ_REMOVE(&obj->list, pg, obj.list);
    pm_free(pg);
  }
  kfree(mpool, obj);
}

void vm_map_remove_entry(vm_map_t *vm_map, vm_map_entry_t *entry) {
  vm_map->nentries--;
  vm_object_free(entry->object);
  TAILQ_REMOVE(&vm_map->list, entry, map_list);
  kfree(mpool, entry);
}

void vm_map_delete(vm_map_t *vm_map) {
  while (vm_map->nentries > 0)
    vm_map_remove_entry(vm_map, TAILQ_FIRST(&vm_map->list));
  kfree(mpool, vm_map);
}

vm_map_entry_t *vm_map_add_entry(vm_map_t *vm_map, uint32_t flags,
                                 size_t length, size_t alignment) {
  assert(is_aligned(length, PAGESIZE));

  vm_map_entry_t *entry = allocate_entry();
  entry->object->size = length;
  entry->flags = flags;
  entry->start = align(vm_map->start, alignment);
  entry->end = entry->start + length;

  vm_map_entry_t *etr_it;
  /* Find place on sorted list, first fit policy */
  TAILQ_FOREACH (etr_it, &vm_map->list, map_list) {
    entry->start = align(etr_it->end, alignment);
    entry->end = entry->start + length;

    if (!TAILQ_NEXT(etr_it, map_list))
      break;

    if (entry->end <= TAILQ_NEXT(etr_it, map_list)->start)
      break;
  }

  insert_entry(vm_map, entry);
  assert(entry->end - entry->start == length);
  return entry;
}

vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr) {
  vm_map_entry_t *etr_it;
  TAILQ_FOREACH (etr_it, &vm_map->list, map_list)
    if (etr_it->start <= vaddr && vaddr < etr_it->end)
      return etr_it;
  return NULL;
}

void vm_map_dump(vm_map_t *vm_map) {
  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &vm_map->list, map_list)
    kprintf("[vm_map] entry (%lu, %lu)\n", it->start, it->end);
}

bool vm_object_add_page(vm_object_t *obj, vm_page_t *page) {
  assert(is_aligned(page->vm_offset, PAGESIZE));
  /* For simplicity of implementation let's insert pages of size 1 only */
  assert(page->size == 1);

  if (!RB_INSERT(vm_object_tree, &obj->tree, page)) {
    obj->npages++;
    vm_page_t *next = RB_NEXT(vm_object_tree, &obj->tree, page);
    if (next)
      TAILQ_INSERT_BEFORE(next, page, obj.list);
    else
      TAILQ_INSERT_TAIL(&obj->list, page, obj.list);
    return true;
  } else {
    return false;
  }
}

void vm_object_remove_page(vm_object_t *obj, vm_page_t *page) {
  TAILQ_REMOVE(&obj->list, page, obj.list);
  RB_REMOVE(vm_object_tree, &obj->tree, page);
  pm_free(page);
  obj->npages--;
}

void vm_map_entry_dump(vm_map_entry_t *entry) {
  vm_page_t *it;
  RB_FOREACH (it, vm_object_tree, &entry->object->tree)
    kprintf("[vm_object] offset: %lu, size: %u \n", it->vm_offset, it->size);
}

vm_page_t *vm_object_find_page(vm_object_t *obj, vm_addr_t offset) {
  vm_page_t find;
  find.vm_offset = offset;
  return RB_FIND(vm_object_tree, &obj->tree, &find);
}

void vm_map_entry_map(vm_map_t *map, vm_map_entry_t *entry, vm_addr_t start,
                      vm_addr_t end, uint8_t flags) {
  assert(map == get_active_vm_map());
  assert(entry->start <= start);
  assert(entry->end >= end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  int npages = (end - start) / PAGESIZE;
  vm_addr_t offset = start - entry->start;

  for (int i = 0; i < npages; i++) {
    vm_page_t *pg = pm_alloc(1);
    pg->vm_offset = offset + i * PAGESIZE;
    pg->prot = flags;
    pmap_map(&map->pmap, entry->start + pg->vm_offset, pg->paddr, 1, flags);
  }
}

void vm_map_entry_unmap(vm_map_t *map, vm_map_entry_t *entry, vm_addr_t start,
                        vm_addr_t end) {
  assert(map == get_active_vm_map());
  assert(entry->start <= start);
  assert(entry->end >= end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  vm_page_t cmp_page = {.vm_offset = start - entry->start};
  vm_page_t *pg_it = RB_NFIND(vm_object_tree, &entry->object->tree, &cmp_page);

  while (pg_it) {
    assert(pg_it->size == 1);
    vm_addr_t page = entry->start + pg_it->vm_offset;

    if ((start > page) || (end < page + PAGESIZE))
      break;

    pg_it->prot = 0;
    pmap_map(&map->pmap, page, 0, 1, 0);
    vm_page_t *tmp = TAILQ_NEXT(pg_it, obj.list);
    vm_object_remove_page(entry->object, pg_it);
    pg_it = tmp;
  }
}

void vm_map_entry_protect(vm_map_t *map, vm_map_entry_t *entry, vm_addr_t start,
                          vm_addr_t end, uint8_t flags) {
  assert(entry->start <= start);
  assert(entry->end >= end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  vm_addr_t end_offset = end - entry->start;

  for (vm_addr_t cur_offset = start - entry->start; cur_offset < end_offset;
       cur_offset += PAGESIZE) {
    vm_page_t *pg = vm_object_find_page(entry->object, cur_offset);
    if (pg) {
      assert(pg->size == 1);
      pg->prot = flags;
    } else {
      vm_page_t *new_pg = pm_alloc_fictitious(1);
      new_pg->prot = flags;
      new_pg->vm_offset = cur_offset;
      vm_object_add_page(entry->object, new_pg);
    }
  }
}

void set_active_vm_map(vm_map_t *map) {
  active_vm_map = map;
}

vm_map_t *get_active_vm_map() {
  return active_vm_map;
}

#ifdef _KERNELSPACE

void paging_on_demand_and_memory_protection_demo() {
  vm_map_t *map = vm_map_new(KERNEL_VM_MAP, 10);
  vm_map_entry_t *entry = vm_map_add_entry(map, 0, 4 * PAGESIZE, PAGESIZE);

  vm_addr_t demand_paged_start = entry->start + PAGESIZE;
  vm_addr_t demand_paged_end = entry->end - PAGESIZE;

  set_active_pmap(&map->pmap);
  set_active_vm_map(map);

  /* Set flags on memory, but don't allocate them, these will be paged on demand
   */
  pmap_map(&map->pmap, demand_paged_start, NULL_PHYS_PAGE,
           (demand_paged_end - demand_paged_start) / PAGESIZE, PMAP_NONE);
  vm_map_entry_protect(map, entry, demand_paged_start, demand_paged_end,
                       PG_READ | PG_WRITE);

  /* Leave page at start and end of range, so access to them causes fault */
  pmap_map(&map->pmap, entry->start, NULL_PHYS_PAGE, 1, PMAP_NONE);
  vm_map_entry_protect(map, entry, entry->start, demand_paged_start, PG_NONE);

  pmap_map(&map->pmap, demand_paged_end - PAGESIZE, NULL_PHYS_PAGE, 1,
           PMAP_NONE);
  vm_map_entry_protect(map, entry, demand_paged_end, entry->end, PG_NONE);

  vm_map_entry_dump(entry);

  /* Start in paged on demand range, but end outside, to cause fault */
  for (vm_addr_t *i = (vm_addr_t *)(demand_paged_start);
       i != (vm_addr_t *)(demand_paged_end); i++) {
    *i = 0xfeedbabe;
  }
}

vm_map_t *prepare_map(asid_t asid, vm_addr_t *entry_start,
                      vm_addr_t *entry_end) {
  vm_map_t *map = vm_map_new(KERNEL_VM_MAP, asid);
  vm_map_entry_t *entry = vm_map_add_entry(map, 0, 5 * PAGESIZE, PAGESIZE);
  *entry_start = entry->start;
  *entry_end = entry->end;

  vm_map_entry_protect(map, entry, entry->start, entry->end,
                       PG_READ | PG_WRITE);
  set_active_pmap(&map->pmap);
  pmap_map(&map->pmap, entry->start, NULL_PHYS_PAGE,
           (entry->end - entry->start) / PAGESIZE, PMAP_NONE);
  return map;
}

void change_virtual_mappings_demo() {
  vm_addr_t entry_start, entry_end;

  vm_map_t *map1 = prepare_map(10, &entry_start, &entry_end);
  vm_map_t *map2 = prepare_map(20, &entry_start, &entry_end);

  int *test_point = (int *)((entry_end - entry_start) / 2 + entry_start);

  /* Set first map, and try to write some value to it*/
  set_active_vm_map(map1);
  set_active_pmap(&map1->pmap);
  *test_point = 0xfeedbabe;

  /* Set second map, and try to write some value to it*/
  set_active_vm_map(map2);
  set_active_pmap(&map2->pmap);
  *test_point = 0xdeadbeef;

  /* Go back to first map, and check if value remained the same */
  set_active_vm_map(map1);
  set_active_pmap(&map1->pmap);
  assert(*test_point == 0xfeedbabe);

  /* Go back to second map, and check if value remained the same */
  set_active_vm_map(map2);
  set_active_pmap(&map2->pmap);
  assert(*test_point == 0xdeadbeef);
}

int main() {
  // paging_on_demand_and_memory_protection_demo();
  change_virtual_mappings_demo();
  return 0;
}

#endif
