#define KL_LOG KL_VM
#include <klog.h>
#include <mman.h>
#include <thread.h>
#include <errno.h>
#include <vm_map.h>
#include <vm_object.h>
#include <mutex.h>
#include <proc.h>

int do_mmap(vm_addr_t *addr_p, size_t length, vm_prot_t prot, int flags) {
  thread_t *td = thread_self();
  assert(td->td_proc != NULL);
  vm_map_t *vmap = td->td_proc->p_uspace;
  assert(vmap != NULL);

  vm_addr_t addr = *addr_p;
  *addr_p = (intptr_t)MAP_FAILED;

  length = roundup(length, PAGESIZE);

  if (!(flags & MMAP_ANON)) {
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

  int error = 0;

  /* Create object with a pager that supplies cleared pages on page fault. */
  vm_object_t *obj = vm_object_alloc(VM_ANONYMOUS);

  if (addr) {
    /* Given the hint try to insert the object exactly at given position. */
    if (vm_map_insert(vmap, obj, addr, addr + length, prot) == NULL)
      error = -ENOMEM;
  } else {
    /* Otherwise let the system choose the best position. */
    if (vm_map_insert_anywhere(vmap, obj, &addr, length, prot) == NULL)
      error = -ENOMEM;
  }

  if (error) {
    vm_object_free(obj);
    return error;
  }

  klog("Created entry at %p, length: %u", (void *)addr, length);

  *addr_p = addr;
  return 0;
}
