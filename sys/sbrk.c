#define KL_LOG KL_PROC
#include <klog.h>
#include <sbrk.h>
#include <common.h>
#include <errno.h>
#include <proc.h>
#include <vm_object.h>

/* Note that this sbrk implementation does not actually extend .data section,
 * because we have no guarantee that there is any free space after .data in the
 * memory map. But it does not matter much, because no application would assume
 * that we are actually expanding .data, it will use the pointer returned by
 * sbrk. */

void sbrk_attach(proc_t *p) {
  assert(p->p_uspace && (p->p_sbrk == NULL));

  vm_map_t *map = p->p_uspace;

  /* Initially allocate one page for brk segment. */
  vm_addr_t addr = SBRK_START;
  vm_object_t *obj = vm_object_alloc(VM_ANONYMOUS);
  vm_map_entry_t *entry = vm_map_insert_anywhere(map, obj, &addr, PAGESIZE,
                                                 VM_PROT_READ | VM_PROT_WRITE);
  assert(entry != NULL);

  p->p_sbrk = entry;
  p->p_sbrk_end = addr;
}

vm_addr_t sbrk_resize(proc_t *p, intptr_t increment) {
  assert(p->p_uspace && p->p_sbrk);

  /* TODO: Shrinking sbrk is impossible, because it requires unmapping pages,
   * which is not yet implemented! */
  if (increment < 0) {
    klog("WARNING: sbrk called with a negative argument!");
    return -ENOMEM;
  }

  vm_map_entry_t *sbrk = p->p_sbrk;
  vm_addr_t last_end = p->p_sbrk_end;
  vm_addr_t entry_end = roundup(p->p_sbrk_end + increment, PAGESIZE);

  /* The segment must be of at least one page. */
  if (entry_end < SBRK_START + PAGESIZE)
    entry_end = SBRK_START + PAGESIZE;

  /* Shrink or expand sbrk vm_map_entry ? */
  if (entry_end != sbrk->end) {
    if (vm_map_resize(p->p_uspace, sbrk, entry_end) != 0)
      return -ENOMEM; /* Map entry expansion failed. */
  }

  p->p_sbrk_end = max(p->p_sbrk_end + increment, SBRK_START);
  return last_end;
}
