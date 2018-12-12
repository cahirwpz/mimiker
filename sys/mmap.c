#define KL_LOG KL_VM
#include <klog.h>
#include <mman.h>
#include <thread.h>
#include <errno.h>
#include <vm_map.h>
#include <vm_object.h>
#include <mutex.h>
#include <proc.h>

int do_mmap(vaddr_t *addr_p, size_t length, vm_prot_t prot, vm_flags_t flags) {
  thread_t *td = thread_self();
  assert(td->td_proc != NULL);
  vm_map_t *vmap = td->td_proc->p_uspace;
  assert(vmap != NULL);

  vaddr_t addr = *addr_p;
  *addr_p = (vaddr_t)MAP_FAILED;
  length = roundup(length, PAGESIZE);

  vm_segment_t *seg;
  int error = vm_map_alloc_segment(vmap, addr, length, prot, flags, &seg);
  if (error < 0)
    return error;

  vaddr_t start, end;
  vm_segment_range(seg, &start, &end);

  klog("Created segment at %p, length: %u", (void *)start, length);

  *addr_p = start;
  return 0;
}
