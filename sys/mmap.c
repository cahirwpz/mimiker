#define KL_LOG KL_VM
#include <klog.h>
#include <mman.h>
#include <thread.h>
#include <errno.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <mutex.h>
#include <proc.h>

vm_addr_t do_mmap(vm_addr_t addr, size_t length, vm_prot_t prot, int flags,
                  int *error) {
  thread_t *td = thread_self();
  assert(td->td_proc);
  vm_map_t *vmap = td->td_proc->p_uspace;

  assert(vmap);

  if (addr >= vmap->pmap->end) {
    /* mmap cannot callocate memory in kernel space! */
    *error = EINVAL;
    return MMAP_FAILED;
  }

  if (!flags & MMAP_ANON) {
    klog("Non-anonymous memory mappings are not yet implemented.");
    *error = EINVAL;
    return MMAP_FAILED;
  }

  length = roundup(length, PAGESIZE);

  /* Regardless of whether addr is 0 or an address hint, we correct it a
     bit. */
  if (addr < MMAP_LOW_ADDR)
    addr = MMAP_LOW_ADDR;
  addr = roundup(addr, PAGESIZE);

  vm_map_entry_t *entry;

  WITH_MTX_LOCK (&vmap->mtx) {
    if (vm_map_findspace_nolock(vmap, addr, length, &addr) != 0) {
      /* No memory was found following the hint. Search again entire address
         space. */
      if (vm_map_findspace_nolock(vmap, MMAP_LOW_ADDR, length, &addr) != 0) {
        /* Still no memory found. */
        *error = ENOMEM;
        return MMAP_FAILED;
      }
    }

    /* Create new vm map entry for this allocation. */
    entry = vm_map_add_entry(vmap, addr, addr + length, prot);
  }

  if (flags & MMAP_ANON) {
    /* Assign a pager which creates cleared pages . */
    entry->object = default_pager->pgr_alloc();
  }

  klog("Created entry at %p, length: %u", (void *)addr, length);

  return addr;
}
