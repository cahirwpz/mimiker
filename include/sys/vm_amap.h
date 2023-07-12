#ifndef _SYS_VM_AMAP_H_
#define _SYS_VM_AMAP_H_

#include <sys/mutex.h>
#include <sys/refcnt.h>
#include <sys/types.h>
#include <sys/vm.h>
#include <stddef.h>

typedef struct vm_amap vm_amap_t;
typedef struct vm_anon vm_anon_t;
typedef struct vm_aref vm_aref_t;

/* Anon structure
 *
 * Marks for fields locks:
 *  (a) atomic
 *  (!) read-only access, do not modify!
 */
struct vm_anon {
  refcnt_t ref_cnt; /* (a) number of references */
  vm_page_t *page;  /* (!) anon page */
};

struct vm_aref {
  size_t offset;   /* offset in slots */
  vm_amap_t *amap; /* underlying amap */
};

/** Allocate new amap.
 *
 * The size of new amap is determined using `slots`.
 *
 * @returns New amap with ref_cnt = 1.
 */
vm_amap_t *vm_amap_alloc(size_t slots);

/** Clone the amap.
 *
 * Create new amap with contents matching old amap. Starting from offset
 * specified by aref and copying specified number of slots.
 *
 * @returns New amap with ref_cnt = 1.
 */
vm_amap_t *vm_amap_clone(vm_aref_t aref, size_t slots);

/** Copy amap when it is needed.
 *
 * Amap is actually copied is referenced by more then one vm_map_entries.
 * Otherwise nothing is done and old aref is returned.
 *
 * @returns Aref ready to use
 */
vm_aref_t vm_amap_needs_copy(vm_aref_t aref, size_t slots);

/** Bump the ref counter to record that amap is used by next one entry. */
void vm_amap_hold(vm_amap_t *amap);

/** Drop ref counter and possibly free amap if it drops to 0. */
void vm_amap_drop(vm_amap_t *amap);

/** Remove multiple pages from amap
 *
 * @param offset Start from this offset in amap.
 * @param n_slots Remove that many pages (anons).
 */
void vm_amap_remove_pages(vm_aref_t aref, size_t offset, size_t n_slots);

/** Get amap reference count */
int vm_amap_ref(vm_amap_t *amap);

/** Get amap max size (in slots) */
size_t vm_amap_slots(vm_amap_t *amap);

/** Alloc new anon.
 *
 * @retval != NULL Anon with zeroed page and ref_cnt = 1.
 * @retval NULL If anon's page can't be allocate.
 */
vm_anon_t *vm_anon_alloc(void);

/** Copy anon.
 *
 * NOTE: Don't decrease ref_cnt of src.
 *
 * @retval != NULL Anon with ref_cnt = 1.
 * @retval NULL If anon's page can't be allocate.
 */
vm_anon_t *vm_anon_copy(vm_anon_t *src);

/** Find anon in amap.
 *
 * @returns Found anon or NULL.
 */
vm_anon_t *vm_amap_find_anon(vm_aref_t aref, size_t offset);

/** Insert anon into amap */
void vm_amap_insert_anon(vm_aref_t aref, vm_anon_t *anon, size_t offset);

/** Replace anon in amap with new one. */
void vm_amap_replace_anon(vm_aref_t aref, vm_anon_t *anon, size_t offset);

/** Bump the ref counter to record that anon is used by one more amap. */
void vm_anon_hold(vm_anon_t *anon);

/** Drop ref counter and possibly free anon if it drops to 0. */
void vm_anon_drop(vm_anon_t *anon);

#endif /* !_SYS_VM_AMAP_H_ */
