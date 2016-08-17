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

static vm_map_entry_t *allocate_entry() {
  vm_map_entry_t *entry = kmalloc(mpool, sizeof(vm_map_entry_t), 0);
  entry->object = NULL;
  return entry;
}

static void vm_map_remove_entry(vm_map_t *vm_map, vm_map_entry_t *entry) {
  vm_map->nentries--;
  vm_object_free(entry->object);
  TAILQ_REMOVE(&vm_map->list, entry, map_list);
  kfree(mpool, entry);
}
vm_object_t *vm_object_allocate() {
  vm_object_t *obj = kmalloc(mpool, sizeof(vm_object_t), 0);
  obj->pgr = pager_allocate(DEFAULT_PAGER);
  TAILQ_INIT(&obj->list);
  RB_INIT(&obj->tree);
  return obj;
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

void set_active_vm_map(vm_map_t *map) {
  active_vm_map = map;
}

vm_map_t *get_active_vm_map() {
  return active_vm_map;
}

void vm_map_init() {
  vm_page_t *pg = pm_alloc(4);
  kmalloc_init(mpool);
  kmalloc_add_arena(mpool, pg->vaddr, PG_SIZE(pg));
  active_vm_map = NULL;
}

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

void vm_map_delete(vm_map_t *vm_map) {
  while (vm_map->nentries > 0)
    vm_map_remove_entry(vm_map, TAILQ_FIRST(&vm_map->list));
  kfree(mpool, vm_map);
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

void vm_object_free(vm_object_t *obj) {
  pager_delete(obj->pgr);
  while (!TAILQ_EMPTY(&obj->list)) {
    vm_page_t *pg = TAILQ_FIRST(&obj->list);
    TAILQ_REMOVE(&obj->list, pg, obj.list);
    pm_free(pg);
  }
  kfree(mpool, obj);
}

vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr) {
  vm_map_entry_t *etr_it;
  TAILQ_FOREACH (etr_it, &vm_map->list, map_list)
    if (etr_it->start <= vaddr && vaddr < etr_it->end)
      return etr_it;
  return NULL;
}

vm_page_t *vm_object_find_page(vm_object_t *obj, vm_addr_t offset) {
  vm_page_t find;
  find.vm_offset = offset;
  return RB_FIND(vm_object_tree, &obj->tree, &find);
}

static vm_map_entry_t* _vm_map_protect(vm_map_t *map, vm_addr_t start, 
        vm_addr_t end, uint8_t prot) {
  
  assert(start >= map->start);
  assert(end < map->end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  /* At the moment we assume user knows what he is doing,
   * In future we want entires to be split */

  assert(vm_map_find_entry(map, start) == NULL);
  assert(vm_map_find_entry(map, end) == NULL);

  vm_map_entry_t *entry = allocate_entry();
  
  entry->start = start;
  entry->end = end;
  entry->prot = prot;

  insert_entry(map, entry);
  return entry;
}

void vm_map_protect(vm_map_t *map, vm_addr_t start, 
        vm_addr_t end, uint8_t prot) {
  _vm_map_protect(map, start, end, prot);
}

/* This function should perhaps use pmap */
void vm_map_insert_object(vm_map_t *map, vm_addr_t start, vm_addr_t end, 
        vm_object_t *object, uint8_t prot) {
  vm_map_entry_t* entry = _vm_map_protect(map, start, end, prot);
  entry->object = object;
}

void vm_map_dump(vm_map_t *vm_map) {
  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &vm_map->list, map_list)
    kprintf("[vm_map] entry (%lu, %lu)\n", it->start, it->end);
}

void vm_map_entry_dump(vm_map_entry_t *entry) {
  vm_page_t *it;
  RB_FOREACH (it, vm_object_tree, &entry->object->tree)
    kprintf("[vm_object] offset: %lu, size: %u \n", it->vm_offset, it->size);
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

#ifdef _KERNELSPACE

void paging_on_demand_and_memory_protection_demo() {
  vm_map_t *map = vm_map_new(USER_VM_MAP, 10);

  vm_addr_t demand_paged_start = 0x1001000;
  vm_addr_t demand_paged_end = 0x1001000 + 2*PAGESIZE;

  set_active_pmap(&map->pmap);
  set_active_vm_map(map);

  /* Add red pages */
  pmap_protect(demand_paged_start-PAGESIZE, 1, PMAP_NONE);
  pmap_protect(demand_paged_end-PAGESIZE, 1, PMAP_NONE);

  vm_map_protect(map, demand_paged_start-PAGESIZE, demand_paged_start, PROT_NONE);
  vm_map_protect(map, demand_paged_end-PAGESIZE, demand_paged_end, PROT_NONE);

  /* Set flags on memory, but don't allocate them, these will be paged on demand
   */
  pmap_protect(demand_paged_start, 2, PMAP_NONE);
  vm_object_t *obj = vm_object_allocate();
  vm_map_insert_object(map, demand_paged_start, demand_paged_end, 
          obj, PROT_READ | PROT_WRITE);

  /* Start in paged on demand range, but end outside, to cause fault */
  for(vm_addr_t *i = (vm_addr_t *)(demand_paged_start);
          i != (vm_addr_t*)(demand_paged_end);
          i++)
  {
      *i = 0xfeedbabe;
  }
}

int main() {
  paging_on_demand_and_memory_protection_demo();
  return 0;
}

#endif
