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
  { KERNEL_VM_MAP, 0xc0400000, 0xe0000000 },
  { USER_VM_MAP,   0x00000000, 0x80000000 },
  { 0 },
};

vm_map_t* vm_map_new(vm_map_type_t type, asid_t asid) {
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

vm_map_entry_t* vm_map_add_entry(vm_map_t* vm_map, uint32_t flags,
                                 size_t length, size_t alignment)
{
  assert(is_aligned(length,PAGESIZE));

  vm_map_entry_t* entry = allocate_entry();
  entry->object->size = length;
  entry->flags = flags;
  entry->start = align(vm_map->start, alignment);
  entry->end = entry->start + length;

  vm_map_entry_t *etr_it;
  /* Find place on sorted list, first fit policy */
  TAILQ_FOREACH(etr_it, &vm_map->list, map_list) {
    entry->start = align(etr_it->end, alignment);
    entry->end = entry->start+length;

    if (!TAILQ_NEXT(etr_it, map_list))
      break;

    if (entry->end <= TAILQ_NEXT(etr_it, map_list)->start)
      break;
  }

  insert_entry(vm_map, entry);
  assert(entry->end-entry->start == length);
  return entry;
}

vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr) {
  vm_map_entry_t *etr_it;
  TAILQ_FOREACH(etr_it, &vm_map->list, map_list)
    if (etr_it->start <= vaddr && vaddr < etr_it->end)
      return etr_it;
  return NULL;
}

void vm_map_dump(vm_map_t *vm_map) {
  vm_map_entry_t *it;
  TAILQ_FOREACH(it, &vm_map->list, map_list)
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
  RB_FOREACH(it, vm_object_tree, &entry->object->tree)
    kprintf("[vm_object] offset: %lu, size: %u \n", it->vm_offset, it->size);
}

vm_page_t* vm_object_find_page(vm_object_t *obj, vm_addr_t offset)
{
  vm_page_t find;
  find.vm_offset = offset;
  return RB_FIND(vm_object_tree, &obj->tree, &find);
}

void vm_map_entry_map(vm_map_t *map, vm_map_entry_t *entry,
                      vm_addr_t start, vm_addr_t end, uint8_t flags)
{
  assert(map == get_active_vm_map());
  assert(&map->pmap == get_active_pmap());
  assert(entry->start <= start);
  assert(entry->end > end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  int npages = (end-start)/PAGESIZE;
  vm_addr_t offset = start-entry->start;

  for(int i = 0; i < npages; i++) {
    vm_page_t *pg = pm_alloc(1);
    pg->vm_offset = offset + i * PAGESIZE;
    pg->vm_flags = flags;
    vm_object_add_page(entry->object, pg);
    pmap_map(&map->pmap, entry->start + pg->vm_offset, pg->paddr, 1, flags);
  }
}

void vm_map_entry_unmap(vm_map_t *map, vm_map_entry_t *entry,
                        vm_addr_t start, vm_addr_t end)
{
  assert(map == get_active_vm_map());
  assert(&map->pmap == get_active_pmap());
  assert(entry->start <= start);
  assert(entry->end > end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  vm_page_t cmp_page = { .vm_offset = start - entry->start };
  vm_page_t *pg_it = RB_NFIND(vm_object_tree, &entry->object->tree, &cmp_page); 

  while (pg_it) {
    assert(pg_it->size == 1);
    vm_addr_t page = entry->start + pg_it->vm_offset;

    if ((start > page) || (end < page + PAGESIZE))
      break;

    pg_it->vm_flags = 0;
    pmap_map(&map->pmap, page, 0, 1, 0);
    vm_page_t *tmp = TAILQ_NEXT(pg_it, obj.list);
    vm_object_remove_page(entry->object, pg_it);
    pg_it = tmp;
  }
}

void vm_map_entry_protect(vm_map_t *map, vm_map_entry_t *entry,
                          vm_addr_t start, vm_addr_t end, uint8_t flags)
{
  assert(map == get_active_vm_map());
  assert(&map->pmap == get_active_pmap());
  assert(entry->start <= start);
  assert(entry->end > end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  pmap_map(&map->pmap, start, 0, (end-start)/PAGESIZE, flags);

  vm_page_t cmp_page = { .vm_offset = start - entry->start };
  vm_page_t *pg_it = RB_NFIND(vm_object_tree, &entry->object->tree, &cmp_page); 

  while (pg_it) {
    assert(pg_it->size == 1);
    vm_addr_t page = entry->start + pg_it->vm_offset;

    if ((start > page) || (end < page + PAGESIZE))
      break;

    pg_it->vm_flags = flags;
    pmap_map(&map->pmap, page, pg_it->paddr, 1, flags);
    pg_it = TAILQ_NEXT(pg_it, obj.list);
  }
}

void set_active_vm_map(vm_map_t* map) {
  active_vm_map = map;
}

vm_map_t* get_active_vm_map() {
  return active_vm_map;
}

#ifdef _KERNELSPACE

int main() {
  return 0;
}

#endif

