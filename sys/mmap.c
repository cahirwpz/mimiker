#define KL_LOG KL_VM
#include <klog.h>
#include <mman.h>
#include <thread.h>
#include <errno.h>
#include <vm_map.h>
#include <vm_object.h>
#include <mutex.h>
#include <proc.h>

int do_mmap(vm_addr_t *addr_p, size_t length, vm_prot_t prot,
            vm_flags_t flags) {
  thread_t *td = thread_self();
  assert(td->td_proc != NULL);
  vm_map_t *vmap = td->td_proc->p_uspace;
  assert(vmap != NULL);

  vm_addr_t addr = *addr_p;
  *addr_p = (intptr_t)MAP_FAILED;

  length = roundup(length, PAGESIZE);

  if (!(flags & VM_ANON)) {
    klog("Only anonymous memory mappings are supported!");
    return -ENOTSUP;
  }

  if (!is_aligned(addr, PAGESIZE))
    return -EINVAL;

  if (length == 0)
    return -EINVAL;

  if (addr != 0 &&
      (!vm_map_in_range(vmap, addr) || !vm_map_in_range(vmap, addr + length)))
    return -EINVAL;

  /* Create object with a pager that supplies cleared pages on page fault. */
  vm_object_t *obj = vm_object_alloc(VM_ANONYMOUS);
  vm_map_entry_t *entry = vm_map_entry_alloc(obj, addr, length, prot);

  /* Given the hint try to insert the object at given position or after it. */
  if (vm_map_insert(vmap, entry, flags)) {
    vm_map_entry_free(entry);
    return -ENOMEM;
  }

  vm_addr_t start, end;
  vm_map_entry_range(entry, &start, &end);

  klog("Created entry at %p, length: %u", (void *)start, length);

  *addr_p = start;
  return 0;
}
