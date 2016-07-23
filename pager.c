#include <pager.h>
#include <physmem.h>

/* Functionality of this pager is to allocate page */
void anon_pager_handler(vm_map_entry_t *entry, vm_addr_t fault_offset)
{
    kprintf("[anon_pager_handler]: %lu\n", fault_offset);

    vm_object_t *obj = entry->object;
    assert(obj != NULL);
    vm_page_t *pg = pm_alloc(1);
    fault_offset = align(fault_offset, PAGESIZE);
    pg->vm_offset = fault_offset;
    pg->vm_flags = 0; //what flags to give to page?
    vm_object_add_page(obj, pg);

    uint8_t pmap_flags = PMAP_VALID;
    if(entry->flags & VM_READ)
        pmap_flags |= PMAP_DIRTY;

    pmap_map(get_active_pmap(), entry->start+fault_offset, pg->paddr, 1, pmap_flags);
}

#ifdef _KERNELSPACE
int main(){

}
#endif

