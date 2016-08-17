#include <pager.h>
#include <physmem.h>
#include <malloc.h>

#define align_down_to_page(addr) ((addr) - ((addr) % PAGESIZE))

static MALLOC_DEFINE(mpool, "inital pager memory pool");

void pager_init() {
  vm_page_t *pg = pm_alloc(4);
  kmalloc_init(mpool);
  kmalloc_add_arena(mpool, pg->vaddr, PG_SIZE(pg));
}

static void *default_pager_handler(vm_object_t *obj,
                                        vm_addr_t fault_addr,
                                        vm_addr_t vm_offset) {
  assert(obj != NULL);
  vm_page_t *new_pg = pm_alloc(1);
  new_pg->vm_offset = vm_offset;

  vm_object_add_page(obj, new_pg);
  return new_pg;
}

void page_fault(vm_map_t *map, vm_addr_t fault_addr, access_t access) {
  vm_map_entry_t *entry = vm_map_find_entry(map, fault_addr);
  if (!entry) {
    panic("Tried to access unmapped memory region: 0x%08lx!\n", fault_addr);
  }

  if (!(entry->prot & PROT_WRITE) && access == WRITE_ACCESS)
    panic("Cannot write to address: 0x%08lx\n", fault_addr);

  if (!(entry->prot & PROT_READ) && access == READ_ACCESS)
    panic("Cannot read from address: 0x%08lx\n", fault_addr);

  assert(entry->start <= fault_addr && fault_addr < entry->end);
  fault_addr = align_down_to_page(fault_addr);
  vm_object_t *object = entry->object;

  assert(object != NULL);

  vm_addr_t offset = fault_addr - entry->start;
  vm_page_t *accessed_page = vm_object_find_page(entry->object, offset);

  if(accessed_page == NULL)
  {
    switch (object->pgr->type) {
        case DEFAULT_PAGER:
          accessed_page = 
            default_pager_handler(object, fault_addr, offset);
          break;
    }
  }
  uint8_t pmap_flags = 0;
  if(entry->prot & PROT_READ)
      pmap_flags |= PMAP_VALID;
  if(entry->prot & PROT_WRITE)
      pmap_flags |= PMAP_DIRTY;
  pmap_map(fault_addr, accessed_page->paddr, 1, pmap_flags);
}

pager_t *pager_allocate(pager_handler_t type) {
  pager_t *pgr = kmalloc(mpool, sizeof(pager_t), 0);
  switch (type) {
  case DEFAULT_PAGER:
    pgr->type = type;
    break;
  }
  return pgr;
}

void pager_delete(pager_t *pgr) {
  switch (pgr->type) {
  case DEFAULT_PAGER:
    /* Nothing happens */
    break;
  }
}
