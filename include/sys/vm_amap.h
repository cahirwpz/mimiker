#ifndef _SYS_VM_AMAP_H_
#define _SYS_VM_AMAP_H_

#include <machine/vm_param.h>

typedef struct vm_amap vm_amap_t;
typedef struct vm_aref vm_aref_t;

struct vm_aref {
  int offset;      /* offset in slots */
  vm_amap_t *amap; /* underlying amap */
};

#define VM_AREF_EMPTY ((vm_aref_t){.offset = 0, .amap = NULL})

/* Allocate new amap with specified number of slots. */
vm_amap_t *vm_amap_alloc(int slots);

int vm_amap_extend(vm_aref_t *aref, int slots);

/*
 * Copy all data from given amap into new one.
 * Returns new amap with ref_cnt equal to 1.
 */
vm_amap_t *vm_amap_clone(vm_aref_t aref);

/* Operations on amap's ref_cnt. */
void vm_amap_hold(vm_amap_t *amap);
void vm_amap_drop(vm_amap_t *amap);

vm_page_t *vm_amap_find_page(vm_aref_t aref, int offset);
int vm_amap_add_page(vm_aref_t aref, vm_page_t *frame, int offset);
void vm_amap_remove_pages(vm_aref_t aref, int offset, int n_slots);

static inline int vaddr_to_slot(vaddr_t addr) {
  return addr / PAGESIZE;
}

int amap_ref(vm_aref_t aref);
int amap_slots(vm_aref_t aref);

#endif /* !_SYS_VM_AMAP_H_ */
