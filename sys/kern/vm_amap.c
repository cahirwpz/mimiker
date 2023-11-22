#define KL_LOG KL_VM
#include <sys/errno.h>
#include <sys/klog.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/refcnt.h>
#include <sys/vm_map.h>
#include <sys/vm_amap.h>
#include <sys/vm_physmem.h>

/*
 * The idea of amaps comes from "Design and Implementation of the UVM Virtual
 * Memory System" by Charles D. Cranor. (https://chuck.cranor.org/p/diss.pdf)
 *
 * The current implementation is a simpler (and limited) version of UVM.
 *
 * Things that are missing now (compared to original UVM Memory System):
 *  - vm_objects (Amaps describe virtual memory. To describe memory mapped files
 *    or devices the vm_objects are needed.)
 *
 * Some limitations of current implementation:
 *  - Amaps are not resizable. We allocate more memory for each amap
 *    (adjustable with EXTRA_AMAP_SLOTS) to allow resizing of small amounts.
 *  - Amap is managing a simple array of referenced pages so it may not be the
 *    most effective implementation.
 *
 * LOCKING
 * There are 2 types of locks already implemented here: amap and anon locks.
 * Sometimes there is a need to hold both mutexes (e.g. when replacing an
 * existing anon in amap with new one). To avoid dead locks the proper order of
 * holding locks must be preserved. The amap should be locked first.
 *
 * Observation: No function needs to be called with any lock held.
 */

/* Amap size will be increased by this number of slots to easier resizing. */
#define EXTRA_AMAP_SLOTS 16

/* Amap structure
 *
 * Marks for fields locks:
 *  (a) atomic
 *  (!) read-only access, do not modify!
 *  (@) guarded by vm-amap::mtx
 */
struct vm_amap {
  mtx_t mtx;             /* Amap lock. */
  size_t slots;          /* (!) maximum number of slots */
  refcnt_t ref_cnt;      /* (a) number map entries using amap */
  vm_anon_t **anon_list; /* (@) pointers of used anons */
};

static POOL_DEFINE(P_VM_AMAP_STRUCT, "vm_amap_struct", sizeof(vm_amap_t));
static POOL_DEFINE(P_VM_ANON_STRUCT, "vm_anon_struct", sizeof(vm_anon_t));
static KMALLOC_DEFINE(M_AMAP, "amap_slots");

int vm_amap_ref(vm_amap_t *amap) {
  return amap->ref_cnt;
}

size_t vm_amap_slots(vm_amap_t *amap) {
  return amap->slots;
}

vm_amap_t *vm_amap_alloc(size_t slots) {
  slots += EXTRA_AMAP_SLOTS;
  vm_amap_t *amap = pool_alloc(P_VM_AMAP_STRUCT, M_WAITOK);

  amap->anon_list =
    kmalloc(M_AMAP, slots * sizeof(vm_anon_t *), M_ZERO | M_WAITOK);

  amap->ref_cnt = 1;
  amap->slots = slots;
  mtx_init(&amap->mtx, MTX_SLEEP);
  return amap;
}

vm_aref_t vm_amap_copy_if_needed(vm_aref_t aref, size_t slots) {
  vm_amap_t *amap = aref.amap;
  if (!amap)
    return (vm_aref_t){.offset = 0, .amap = NULL};

  vm_amap_t *new;
  WITH_MTX_LOCK (&amap->mtx) {
    /* XXX: This check is actually safe. If amap has only one reference it means
     * that it is reference of current vm_map_entry. Ref_cnt can't be bumped
     * now, because it is done only in syscalls (fork, munmap and mprotect) (and
     * we are currently handling page fault so we can't be in such syscall).
     * That is because during these syscalls we can't cause page fault on
     * currently accessed vm_map_entry.
     */

    /* Amap don't have to be copied. */
    if (amap->ref_cnt == 1)
      return aref;

    new = vm_amap_alloc(slots);
    for (size_t slot = 0; slot < slots; slot++) {
      size_t old_slot = aref.offset + slot;

      if (!amap->anon_list[old_slot])
        continue;

      vm_anon_t *anon = amap->anon_list[old_slot];

      vm_anon_hold(anon);
      new->anon_list[slot] = anon;
    }
  }
  vm_amap_drop(amap);
  return (vm_aref_t){.offset = 0, .amap = new};
}

vm_anon_t *vm_amap_find_anon(vm_aref_t aref, size_t offset) {
  vm_amap_t *amap = aref.amap;
  assert(amap != NULL);

  /* Determine real offset inside the amap. */
  offset += aref.offset;
  assert(offset < amap->slots);

  SCOPED_MTX_LOCK(&amap->mtx);
  if (amap->anon_list[offset])
    return amap->anon_list[offset];
  return NULL;
}

void vm_amap_insert_anon(vm_aref_t aref, vm_anon_t *anon, size_t offset) {
  vm_amap_t *amap = aref.amap;
  assert(amap != NULL && anon != NULL);

  SCOPED_MTX_LOCK(&amap->mtx);

  /* Determine real offset inside the amap. */
  offset += aref.offset;
  assert(offset < amap->slots);

  /* Don't allow for inserting anon twice or on top of other. */
  if (amap->anon_list[offset]) {
    assert(anon == amap->anon_list[offset]);
    return;
  }

  amap->anon_list[offset] = anon;
}

static void vm_amap_remove_pages_unlocked(vm_amap_t *amap, size_t start,
                                          size_t nslots) {

  /* XXX: It was previously done by gathering all pages to free on a linked
   * list. Due to additional layer of anons it is harder to do. Leave it as it
   * is for now, but need to revisit it. */

  for (size_t i = start; i < start + nslots; i++) {
    if (!amap->anon_list[i])
      continue;
    vm_anon_drop(amap->anon_list[i]);
    amap->anon_list[i] = NULL;
  }
}

void vm_amap_remove_pages(vm_aref_t aref, size_t start, size_t nslots) {
  if (!aref.amap)
    return;
  SCOPED_MTX_LOCK(&aref.amap->mtx);
  vm_amap_remove_pages_unlocked(aref.amap, aref.offset + start, nslots);
}

void vm_amap_hold(vm_amap_t *amap) {
  refcnt_acquire(&amap->ref_cnt);
}

void vm_amap_drop(vm_amap_t *amap) {
  if (refcnt_release(&amap->ref_cnt)) {
    vm_amap_remove_pages_unlocked(amap, 0, amap->slots);
    kfree(M_AMAP, amap->anon_list);
    pool_free(P_VM_AMAP_STRUCT, amap);
  }
}

static vm_anon_t *alloc_empty_anon(void) {
  vm_anon_t *anon = pool_alloc(P_VM_ANON_STRUCT, M_WAITOK);
  anon->ref_cnt = 1;
  anon->page = vm_page_alloc(1);
  if (!anon->page) {
    pool_free(P_VM_ANON_STRUCT, anon);
    return NULL;
  }
  return anon;
}

vm_anon_t *vm_anon_alloc(void) {
  vm_anon_t *anon = alloc_empty_anon();
  if (!anon)
    return NULL;
  pmap_zero_page(anon->page);
  return anon;
}

void vm_anon_hold(vm_anon_t *anon) {
  refcnt_acquire(&anon->ref_cnt);
}

void vm_anon_drop(vm_anon_t *anon) {
  if (refcnt_release(&anon->ref_cnt)) {
    vm_page_free(anon->page);
    pool_free(P_VM_ANON_STRUCT, anon);
  }
}

vm_anon_t *vm_anon_copy(vm_anon_t *src) {
  vm_anon_t *new = alloc_empty_anon();
  if (!new)
    return NULL;
  pmap_copy_page(src->page, new->page);
  return new;
}
