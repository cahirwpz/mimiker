#include <stdc.h>
#include <vm_pager.h>
#include <vm_object.h>
#include <vm_map.h>

static void paging_on_demand_and_memory_protection_demo() {
  set_active_vm_map(vm_map_new(USER_VM_MAP));

  vm_map_t *kmap = get_active_vm_map(PMAP_KERNEL);
  vm_map_t *umap = get_active_vm_map(PMAP_USER);

  log("Kernel physical map : %08lx-%08lx", kmap->pmap.start, kmap->pmap.end);
  log("User physical map   : %08lx-%08lx", umap->pmap.start, umap->pmap.end);

  vm_addr_t start = 0x1001000;
  vm_addr_t end = 0x1001000 + 2 * PAGESIZE;

  vm_map_entry_t *redzone0 =
    vm_map_add_entry(umap, start - PAGESIZE, start, VM_PROT_NONE);
  vm_map_entry_t *redzone1 =
    vm_map_add_entry(umap, end, end + PAGESIZE, VM_PROT_NONE);
  vm_map_entry_t *data =
    vm_map_add_entry(umap, start, end, VM_PROT_READ | VM_PROT_WRITE);

  redzone0->object = vm_object_alloc();
  redzone1->object = vm_object_alloc();
  data->object = default_pager->pgr_alloc();

  vm_map_dump(umap);
  vm_map_dump(kmap);

  /* Start in paged on demand range, but end outside, to cause fault */
  for (int *ptr = (int *)start; ptr != (int *)end; ptr += 256) {
    log("%p", ptr);
    *ptr = 0xfeedbabe;
  }

  vm_map_dump(umap);
  vm_map_dump(kmap);

  log("Test passed.");
}

int main() {
  paging_on_demand_and_memory_protection_demo();
  return 0;
}
