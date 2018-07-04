#define KL_LOG KL_PROC
#include <klog.h>
#include <sbrk.h>
#include <errno.h>
#include <proc.h>
#include <vm_object.h>

/* Note that this sbrk implementation does not actually extend .data section,
 * because we have no guarantee that there is any free space after .data in the
 * memory map. But it does not matter much, because no application would assume
 * that we are actually expanding .data, it will use the pointer returned by
 * sbrk. */
/* TODO: make sbrk expand .bss segment. */

void sbrk_attach(proc_t *p) {
  assert(p->p_uspace && (p->p_sbrk == NULL));

  vm_map_t *map = p->p_uspace;

  /* Initially allocate one page for brk segment. */
  vaddr_t addr = SBRK_START;
  vm_object_t *obj = vm_object_alloc(VM_ANONYMOUS);
  vm_segment_t *seg =
    vm_segment_alloc(obj, addr, addr + PAGESIZE, VM_PROT_READ | VM_PROT_WRITE);
  if (vm_map_insert(map, seg, VM_FIXED))
    panic("Could not allocate data segment!");

  p->p_sbrk = seg;
  p->p_sbrk_end = addr;
}

vaddr_t sbrk_resize(proc_t *p, intptr_t increment) {
  assert(p->p_uspace && p->p_sbrk);

  if (increment == 0)
    return p->p_sbrk_end;

  vaddr_t last_end = p->p_sbrk_end;
  vaddr_t new_end = p->p_sbrk_end + increment;

  vaddr_t sbrk_start, sbrk_end;
  vm_segment_range(p->p_sbrk, &sbrk_start, &sbrk_end);

  if (new_end < sbrk_start)
    return -EINVAL;

  /* TODO: Shrinking sbrk is impossible, because it requires unmapping pages,
   * which is not yet implemented! */
  if (new_end < last_end)
    return -ENOTSUP;

  /* Expand segment break! */
  if (vm_map_resize(p->p_uspace, p->p_sbrk, roundup(new_end, PAGESIZE)) != 0)
    return -ENOMEM; /* Segment expansion failed. */

  p->p_sbrk_end = new_end;
  return last_end;
}
