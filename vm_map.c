#include <malloc.h>
#include <libkern.h>
#include <vm_pager.h>
#include <vm_object.h>
#include <vm_map.h>

static vm_map_t *active_vm_map;

void set_active_vm_map(vm_map_t *map) { active_vm_map = map; }
vm_map_t *get_active_vm_map() { return active_vm_map; }

static inline int vm_map_entry_cmp(vm_map_entry_t *a, vm_map_entry_t *b) {
  if (a->start < b->start)
    return -1;
  return a->start - b->start;
}

SPLAY_PROTOTYPE(vm_map_tree, vm_map_entry, map_tree, vm_map_entry_cmp);
SPLAY_GENERATE(vm_map_tree, vm_map_entry, map_tree, vm_map_entry_cmp);

static MALLOC_DEFINE(mpool, "vm_map memory pool");

void vm_map_init() {
  vm_page_t *pg = pm_alloc(2);
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
  vm_map_t *vm_map = kmalloc(mpool, sizeof(vm_map_t), M_ZERO);

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

static bool vm_map_insert_entry(vm_map_t *vm_map, vm_map_entry_t *entry) {
  if (!SPLAY_INSERT(vm_map_tree, &vm_map->tree, entry)) {
    vm_map_entry_t *next = SPLAY_NEXT(vm_map_tree, &vm_map->tree, entry);
    if (next)
      TAILQ_INSERT_BEFORE(next, entry, map_list);
    else
      TAILQ_INSERT_TAIL(&vm_map->list, entry, map_list);
    vm_map->nentries++;
    return true;
  }
  return false;
}

vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr) {
  vm_map_entry_t *etr_it;
  TAILQ_FOREACH (etr_it, &vm_map->list, map_list)
    if (etr_it->start <= vaddr && vaddr < etr_it->end)
      return etr_it;
  return NULL;
}

static void vm_map_remove_entry(vm_map_t *vm_map, vm_map_entry_t *entry) {
  vm_map->nentries--;
  vm_object_free(entry->object);
  TAILQ_REMOVE(&vm_map->list, entry, map_list);
  kfree(mpool, entry);
}

void vm_map_delete(vm_map_t *map) {
  while (map->nentries > 0)
    vm_map_remove_entry(map, TAILQ_FIRST(&map->list));
  kfree(mpool, map);
}

static vm_map_entry_t *_vm_map_protect(vm_map_t *map, vm_addr_t start,
                                       vm_addr_t end, vm_prot_t prot) {
  assert(start >= map->start);
  assert(end < map->end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  /* At the moment we assume user knows what he is doing,
   * In future we want entires to be split */

  assert(vm_map_find_entry(map, start) == NULL);
  assert(vm_map_find_entry(map, end) == NULL);

  vm_map_entry_t *entry = kmalloc(mpool, sizeof(vm_map_entry_t), M_ZERO);

  entry->start = start;
  entry->end = end;
  entry->prot = prot;

  vm_map_insert_entry(map, entry);
  return entry;
}

void vm_map_protect(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                    vm_prot_t prot) {
  _vm_map_protect(map, start, end, prot);
}

/* This function should perhaps use pmap */
void vm_map_insert_object(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                          vm_object_t *object, vm_prot_t prot) {
  vm_map_entry_t *entry = _vm_map_protect(map, start, end, prot);
  entry->object = object;
}

void vm_map_dump(vm_map_t *vm_map) {
  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &vm_map->list, map_list) {
    kprintf("[vm_map] entry (%lu, %lu)\n", it->start, it->end);
    vm_map_object_dump(it->object);
  }
}

void vm_page_fault(vm_map_t *map, vm_addr_t fault_addr, vm_prot_t fault_type) {
  vm_map_entry_t *entry;

  if (!(entry = vm_map_find_entry(map, fault_addr)))
    panic("Tried to access unmapped memory region: 0x%08lx!\n", fault_addr);

  if (!(entry->prot & VM_PROT_WRITE) && (fault_type == VM_PROT_WRITE))
    panic("Cannot write to address: 0x%08lx\n", fault_addr);

  if (!(entry->prot & VM_PROT_READ) && (fault_type == VM_PROT_READ))
    panic("Cannot read from address: 0x%08lx\n", fault_addr);

  assert(entry->start <= fault_addr && fault_addr < entry->end);

  vm_object_t *obj = entry->object;

  assert(obj != NULL);

  vm_addr_t fault_page = fault_addr & -PAGESIZE;
  vm_addr_t offset = fault_page - entry->start;
  vm_page_t *accessed_page = vm_object_find_page(entry->object, offset);

  if (accessed_page == NULL)
    accessed_page = obj->pgr->pgr_fault(obj, fault_page, offset, fault_type);
  vm_prot_t pmap_flags = 0;
  if (entry->prot & VM_PROT_READ)
    pmap_flags |= PMAP_VALID;
  if (entry->prot & VM_PROT_WRITE)
    pmap_flags |= PMAP_DIRTY;
  pmap_map(fault_addr, accessed_page->paddr, 1, pmap_flags);
}

#ifdef _KERNELSPACE

void paging_on_demand_and_memory_protection_demo() {
  vm_map_t *map = vm_map_new(USER_VM_MAP, 10);

  vm_addr_t demand_paged_start = 0x1001000;
  vm_addr_t demand_paged_end = 0x1001000 + 2 * PAGESIZE;

  set_active_pmap(&map->pmap);
  set_active_vm_map(map);

  /* Add red pages */
  pmap_protect(demand_paged_start - PAGESIZE, 1, PMAP_NONE);
  pmap_protect(demand_paged_end - PAGESIZE, 1, PMAP_NONE);

  vm_map_protect(map, demand_paged_start - PAGESIZE, demand_paged_start,
                 VM_PROT_NONE);
  vm_map_protect(map, demand_paged_end - PAGESIZE, demand_paged_end,
                 VM_PROT_NONE);

  /* Set flags on memory, but don't allocate them, these will be paged on demand
   */
  pmap_protect(demand_paged_start, 2, PMAP_NONE);
  vm_object_t *obj = vm_object_alloc();
  vm_map_insert_object(map, demand_paged_start, demand_paged_end, obj,
                       VM_PROT_READ | VM_PROT_WRITE);

  /* Start in paged on demand range, but end outside, to cause fault */
  for (vm_addr_t *i = (vm_addr_t *)(demand_paged_start);
       i != (vm_addr_t *)(demand_paged_end); i++) {
    *i = 0xfeedbabe;
  }
}

int main() {
  paging_on_demand_and_memory_protection_demo();
  return 0;
}

#endif
