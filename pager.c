#include <pager.h>
#include <physmem.h>
#include <malloc.h>

#define align_down_to_page(addr) ((addr)-((addr) % PAGESIZE))

static MALLOC_DEFINE(mpool, "inital pager memory pool");

void pager_init()
{
    vm_page_t *pg = pm_alloc(4);
    kmalloc_init(mpool);
    kmalloc_add_arena(mpool, pg->vaddr, PG_SIZE(pg));
}

static vm_page_t* default_pager_handler(vm_object_t *obj, vm_addr_t offset, vm_addr_t fault_addr) {
    assert(obj != NULL);
    kprintf("[default_pager]: 0x%08lx\n", fault_addr);

    vm_page_t *pg = pm_alloc(1);
    pg->vm_offset = offset;
    pg->vm_flags = PG_VALID;

    vm_object_add_page(obj, pg);
    pmap_map(get_active_pmap(), fault_addr, pg->paddr, 1, PMAP_VALID);
    return vm_object_find_page(obj, offset);
}

void page_fault(vm_map_t *map, vm_addr_t fault_addr, access_t access)
{
    vm_map_entry_t *entry = vm_map_find_entry(map, fault_addr);
    if(!entry)
    {
        panic("Tried to access unmapped memory region: 0x%08x!\n", (unsigned)fault_addr);
    }
    
    /* Check flags */
    if(access == READ_ACCESS && !(entry->flags & VM_READ))
    {
        panic("Cannot read from that address: 0x%08x!\n", (unsigned)fault_addr);
    }

    if(access == WRITE_ACCESS && !(entry->flags & VM_WRITE))
    {
        panic("Cannot write to that address: 0x%08x!\n", (unsigned)fault_addr);
    }


    assert(entry->start <= fault_addr && fault_addr < entry->end);
    fault_addr = align_down_to_page(fault_addr);
    vm_object_t *object = entry->object;
    assert(object != NULL);
    vm_addr_t offset = fault_addr - entry->start;
    vm_page_t *accessed_page = NULL;

    switch(object->pgr->type)
    {
        case DEFAULT_PAGER:
                accessed_page = default_pager_handler(object, offset, fault_addr);
        break;
    }

    if(access == WRITE_ACCESS && accessed_page)
    {
        accessed_page->vm_flags |= PG_DIRTY;
        pmap_map(get_active_pmap(), fault_addr, accessed_page->paddr, 1, PMAP_VALID | PMAP_DIRTY);
    }
}

pager_t* pager_allocate(pager_handler_t type)
{
    pager_t *pgr = kmalloc(mpool, sizeof(pager_t), 0);
    switch(type)
    {
        case DEFAULT_PAGER:
            pgr->type = type;
            break;
    }
    return pgr;
}

void pager_delete(pager_t* pgr)
{
    switch(pgr->type)
    {
        case DEFAULT_PAGER:
            /* Nothing happens */
            break;
    }
}

