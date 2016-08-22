#include <stdc.h>
#include <malloc.h>
#include <vm_object.h>
#include <vm_pager.h>

static vm_object_t *default_pager_alloc() {
  vm_object_t *obj = vm_object_alloc();
  obj->pgr = (pager_t *)default_pager;
  return obj;
}

static void default_pager_free(vm_object_t *obj) {
  vm_object_free(obj);
}

static vm_page_t *default_pager_fault(vm_object_t *obj, vm_addr_t fault_addr,
                                      vm_addr_t vm_offset, vm_prot_t vm_prot) {
  assert(obj != NULL);

  vm_page_t *new_pg = pm_alloc(1);
  new_pg->vm_offset = vm_offset;
  vm_object_add_page(obj, new_pg);
  return new_pg;
}

pager_t default_pager[1] = {{
  .pgr_alloc = default_pager_alloc,
  .pgr_free = default_pager_free,
  .pgr_fault = default_pager_fault,
}};
