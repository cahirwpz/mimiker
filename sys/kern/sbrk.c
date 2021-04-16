#define KL_LOG KL_PROC
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/sbrk.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/uvm_object.h>

/* Note that this sbrk implementation does not actually extend .data section,
 * because we have no guarantee that there is any free space after .data in the
 * memory map. But it does not matter much, because no application would assume
 * that we are actually expanding .data, it will use the pointer returned by
 * sbrk. */
/* TODO: make sbrk expand .bss segment. */

void sbrk_attach(proc_t *p) {
  assert(proc_self() == p);
  assert(p->p_uspace && (p->p_sbrk == NULL));

  vm_map_t *map = p->p_uspace;

  /* Initially allocate one page for brk segment. */
  vaddr_t addr = SBRK_START;
  uvm_object_t *obj = uvm_object_alloc(VM_ANONYMOUS);
  vm_map_entry_t *ent = vm_map_entry_alloc(
    obj, addr, addr + PAGESIZE, VM_PROT_READ | VM_PROT_WRITE, VM_ENT_PRIVATE);
  if (vm_map_insert(map, ent, VM_FIXED))
    panic("Could not allocate data segment!");

  p->p_sbrk = ent;
  p->p_sbrk_end = addr;
}

int sbrk_resize(proc_t *p, intptr_t increment, vaddr_t *newbrkp) {
  assert(proc_self() == p);
  assert(p->p_uspace && p->p_sbrk);

  vaddr_t last_end = p->p_sbrk_end;
  vaddr_t new_end = p->p_sbrk_end + increment;
  vaddr_t sbrk_start = vm_map_entry_start(p->p_sbrk);

  if (new_end < sbrk_start)
    return EINVAL;

  /* require sbrk_segment to contain at least one page */
  vaddr_t new_end_aligned =
    max(align(new_end, PAGESIZE), sbrk_start + PAGESIZE);

  if (vm_map_entry_resize(p->p_uspace, p->p_sbrk, new_end_aligned))
    return ENOMEM;

  p->p_sbrk_end = new_end;

  *newbrkp = last_end;
  return 0;
}
