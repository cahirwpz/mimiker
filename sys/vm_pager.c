#include <stdc.h>
#include <physmem.h>
#include <vm_object.h>
#include <vm_pager.h>

static vm_page_t *dummy_pager_fault(vm_object_t *obj, vaddr_t fault_addr,
                                    off_t offset, vm_prot_t vm_prot) {
  return NULL;
}

static vm_page_t *anon_pager_fault(vm_object_t *obj, vaddr_t fault_addr,
                                   off_t offset, vm_prot_t vm_prot) {
  assert(obj != NULL);

  vm_page_t *new_pg = pm_alloc(1);
  new_pg->offset = offset;
  bzero((char *)new_pg->vaddr, PAGESIZE);
  vm_object_add_page(obj, new_pg);
  return new_pg;
}

vm_pager_t pagers[] = {
    [VM_DUMMY] = {.pgr_fault = dummy_pager_fault},
    [VM_ANONYMOUS] = {.pgr_fault = anon_pager_fault},
};
