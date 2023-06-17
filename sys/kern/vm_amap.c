#include "sys/mutex.h"
#define KL_LOG KL_VM
#include <sys/klog.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/vm_map.h>
#include <sys/vm_amap.h>
#include <sys/vm_physmem.h>
#include <sys/errno.h>

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
 * There are 2 types of locks already implemented here: amapa and anon locks.
 * Sometimes there is a need to hold both mutexes (e.g. when replacing an
 * existing anon in amap with new one). To avoid dead locks the proper order of
 * holding locks must be preserved. The amap should be locked first.
 *
 * Few observations about locking:
 *  1. No function needs to be called with any lock held.
 *  2. The only situation when we need to hold 2 locks is in
 * `vm_amap_replace_anon`.
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
  uint32_t ref_cnt;      /* (a) number of references */
  vm_anon_t **anon_list; /* (@) anon list */
  bitstr_t *anon_bitmap; /* (@) anon bitmap */
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

  amap->anon_bitmap = kmalloc(M_AMAP, bitstr_size(slots), M_ZERO | M_WAITOK);

  amap->ref_cnt = 1;
  amap->slots = slots;
  mtx_init(&amap->mtx, MTX_SLEEP);
  return amap;
}

vm_aref_t vm_amap_needs_copy(vm_aref_t aref, size_t slots) {
  vm_amap_t *amap = aref.amap;
  if (!amap)
    return (vm_aref_t){.amap = NULL, .offset = 0};

  SCOPED_MTX_LOCK(&amap->mtx);

  /* Amap don't have to be copied. */
  if (amap->ref_cnt == 1) {
    return aref;
  }

  amap->ref_cnt--;

  vm_amap_t *new = vm_amap_alloc(slots);
  for (size_t slot = 0; slot < slots; slot++) {
    size_t old_slot = aref.offset + slot;

    if (!bit_test(amap->anon_bitmap, old_slot))
      continue;

    vm_anon_t *anon = amap->anon_list[old_slot];

    vm_anon_hold(anon);
    new->anon_list[slot] = anon;
    bit_set(new->anon_bitmap, slot);
  }
  return (vm_aref_t){.offset = 0, .amap = new};
}

vm_anon_t *vm_amap_find_anon(vm_aref_t aref, size_t offset) {
  vm_amap_t *amap = aref.amap;
  assert(amap != NULL);

  /* Determine real offset inside the amap. */
  offset += aref.offset;
  assert(offset < amap->slots);

  SCOPED_MTX_LOCK(&amap->mtx);
  if (bit_test(amap->anon_bitmap, offset))
    return amap->anon_list[offset];
  return NULL;
}

void vm_amap_insert_anon(vm_aref_t aref, vm_anon_t *anon, size_t offset) {
  vm_amap_t *amap = aref.amap;
  assert(amap != NULL && anon != NULL);

  /* Determine real offset inside the amap. */
  offset += aref.offset;
  assert(offset < amap->slots);

  SCOPED_MTX_LOCK(&amap->mtx);

  /* We expect empty slot. */
  assert(!bit_test(amap->anon_bitmap, offset));

  amap->anon_list[offset] = anon;
  bit_set(amap->anon_bitmap, offset);
}

bool vm_amap_replace_anon(vm_aref_t aref, vm_anon_t *anon, size_t offset) {
  vm_amap_t *amap = aref.amap;
  assert(amap != NULL && anon != NULL);

  /* Determine real offset inside the amap. */
  offset += aref.offset;
  assert(offset < amap->slots);

  SCOPED_MTX_LOCK(&amap->mtx);

  /* Anon must be already there. */
  assert(bit_test(amap->anon_bitmap, offset));

  vm_anon_t *old = amap->anon_list[offset];

  /* If old anon was transferred to another amap there is no need to
   * replace it here. */
  WITH_MTX_LOCK (&old->mtx) {
    if (old->ref_cnt == 1) {
      vm_anon_drop(anon);
      return false;
    }
    /* We are actually replacing it. */
    old->ref_cnt--;
  }

  amap->anon_list[offset] = anon;
  return true;
}

static void vm_amap_remove_pages_unlocked(vm_amap_t *amap, size_t start,
                                          size_t nslots) {

  /* XXX: It was previously done by gathering all pages to free on a linked
   * list. Due to additional layer of anons it is harder to do. Leave it as it
   * is for now, but need to revisit it. */

  for (size_t i = start; i < start + nslots; i++) {
    if (!bit_test(amap->anon_bitmap, i))
      continue;
    vm_anon_drop(amap->anon_list[i]);
    bit_clear(amap->anon_bitmap, i);
  }
}

void vm_amap_remove_pages(vm_aref_t aref, size_t start, size_t nslots) {
  if (!aref.amap)
    return;
  SCOPED_MTX_LOCK(&aref.amap->mtx);
  vm_amap_remove_pages_unlocked(aref.amap, aref.offset + start, nslots);
}

void vm_amap_hold(vm_amap_t *amap) {
  SCOPED_MTX_LOCK(&amap->mtx);
  amap->ref_cnt++;
}

void vm_amap_drop(vm_amap_t *amap) {
  WITH_MTX_LOCK (&amap->mtx) {
    amap->ref_cnt--;
    if (amap->ref_cnt >= 1)
      return;
  }
  vm_amap_remove_pages_unlocked(amap, 0, amap->slots);
  kfree(M_AMAP, amap->anon_list);
  kfree(M_AMAP, amap->anon_bitmap);
  pool_free(P_VM_AMAP_STRUCT, amap);
}

static vm_anon_t *alloc_empty_anon(void) {
  vm_anon_t *anon = pool_alloc(P_VM_ANON_STRUCT, M_WAITOK);
  anon->ref_cnt = 1;
  anon->page = NULL;
  mtx_init(&anon->mtx, MTX_SLEEP);
  return anon;
}

vm_anon_t *vm_anon_alloc(void) {
  vm_anon_t *anon = alloc_empty_anon();
  vm_page_t *page = vm_page_alloc(1);
  if (!page) {
    vm_anon_drop(anon);
    return NULL;
  }
  pmap_zero_page(page);
  anon->page = page;
  return anon;
}

void vm_anon_hold(vm_anon_t *anon) {
  SCOPED_MTX_LOCK(&anon->mtx);
  anon->ref_cnt++;
}

void vm_anon_drop(vm_anon_t *anon) {
  WITH_MTX_LOCK (&anon->mtx) {
    anon->ref_cnt--;
    if (anon->ref_cnt >= 1)
      return;
  }
  vm_page_free(anon->page);
  pool_free(P_VM_ANON_STRUCT, anon);
}

vm_anon_t *vm_anon_copy(vm_anon_t *src) {
  vm_anon_t *new = alloc_empty_anon();
  new->page = vm_page_alloc(1);
  pmap_copy_page(src->page, new->page);
  return new;
}
