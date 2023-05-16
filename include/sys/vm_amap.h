#ifndef _SYS_VM_AMAP_H_
#define _SYS_VM_AMAP_H_

#include <sys/refcnt.h>
#include <sys/types.h>
#include <sys/vm.h>
#include <stddef.h>

typedef struct vm_amap vm_amap_t;
typedef struct vm_anon vm_anon_t;
typedef struct vm_aref vm_aref_t;

struct vm_anon {
  refcnt_t ref_cnt;
  vm_page_t *page;
};

struct vm_aref {
  size_t offset;   /* offset in slots */
  vm_amap_t *amap; /* underlying amap */
};

/*
 * Allocate new amap with specified number of slots.
 *
 * Always returns new amap with ref_cnt equal to 1.
 */
vm_amap_t *vm_amap_alloc(size_t slots);

/*
 * Create new amap with contents matching old amap. Starting from offset
 * specified by aref and copying specified number of slots.
 *
 * Always returns new amap with ref_cnt equal to 1.
 */
vm_amap_t *vm_amap_clone(vm_aref_t aref, size_t slots);

/*
 * Copy amap if it is referenced by more then one vm_map_entries.
 *
 * Returns aref to new amap.
 */
vm_aref_t vm_amap_needs_copy(vm_aref_t aref, size_t slots);

/* Bump the ref counter to record that amap is used by next one entry. */
void vm_amap_hold(vm_amap_t *amap);

/* Drop ref counter and possibly free amap if it drops to 0. */
void vm_amap_drop(vm_amap_t *amap);

void vm_amap_remove_pages(vm_aref_t aref, size_t offset, size_t n_slots);

/* Functions accessing amap's internal data */
int vm_amap_ref(vm_amap_t *amap);
size_t vm_amap_slots(vm_amap_t *amap);

/* Alloc anon with ref_cnt set to 1 */
vm_anon_t *vm_anon_alloc(void);
vm_anon_t *vm_anon_transfer(vm_anon_t *src);

vm_anon_t *vm_amap_find_anon(vm_aref_t aref, size_t offset);

void vm_amap_insert_anon(vm_aref_t aref, vm_anon_t *anon, size_t offset);

void vm_amap_replace_anon(vm_aref_t aref, vm_anon_t *anon, size_t offset);

/* Operations on anon's ref_cnt. */
void vm_anon_hold(vm_anon_t *anon);
void vm_anon_drop(vm_anon_t *anon);

#endif /* !_SYS_VM_AMAP_H_ */
