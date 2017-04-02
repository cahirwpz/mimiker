#include <sbrk.h>
#include <common.h>
#include <errno.h>
#include <vm_pager.h>

/* Note that this sbrk implementation does not actually extend .data section,
   because we have no guarantee that there is any free space after .data in the
   memory map. But it does not matter much, because no application would assume
   that we are actually expanding .data, it will use the pointer returned by
   sbrk. */

vm_addr_t sbrk_create(vm_map_t *map) {
  assert(!map->sbrk_entry);

  size_t size = roundup(SBRK_INITIAL_SIZE, PAGESIZE);
  vm_addr_t addr;
  assert(vm_map_findspace(map, BRK_SEARCH_START, size, &addr) == 0);
  vm_map_entry_t *entry =
    vm_map_add_entry(map, addr, addr + size, VM_PROT_READ | VM_PROT_WRITE);
  entry->object = default_pager->pgr_alloc();

  map->sbrk_entry = entry;
  map->sbrk_end = addr;

  return addr;
}

vm_addr_t sbrk_resize(vm_map_t *map, intptr_t increment) {
  assert(map->sbrk_entry);

  vm_map_entry_t *brk_entry = map->sbrk_entry;
  vm_addr_t brk = map->sbrk_end;
  if (brk + increment == brk_entry->end) {
    /* No need to resize the segment. */
    map->sbrk_end = brk + increment;
    return brk;
  }
  /* Shrink or expand the vm_map_entry */
  vm_addr_t new_end = roundup(brk + increment, PAGESIZE);
  if (vm_map_resize(map, brk_entry, new_end) != 0) {
    /* Map entry expansion failed. */
    return -ENOMEM;
  }
  map->sbrk_end += increment;
  return brk;
}
