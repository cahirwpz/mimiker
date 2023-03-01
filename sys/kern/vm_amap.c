#define KL_LOG KL_VM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/vm_map.h>
#include <sys/vm_amap.h>
#include <sys/vm_physmem.h>
#include <sys/refcnt.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/pcpu.h>
#include <machine/vm_param.h>

/* Every amap will be bigger by this amount of slots to make resizing possible
 */
#define EXTRA_AMAP_SLOTS 16

struct vm_amap {
  int slots;        /* Read-only. Do not modify. */
  refcnt_t ref_cnt; /* Atomic. */
  mtx_t mtx;        /* Mutex quarding page list and bitmap. */
  vm_page_t **pg_list;
  bitstr_t *pg_bitmap;
};

static void vm_amap_free(vm_amap_t *amap);

static POOL_DEFINE(P_VM_AMAP_STRUCT, "vm_amap_struct", sizeof(vm_amap_t));
static KMALLOC_DEFINE(M_AMAP, "amap_slots");

int vm_amap_ref(vm_amap_t *amap) {
  return amap->ref_cnt;
}

int vm_amap_slots(vm_amap_t *amap) {
  return amap->slots;
}

vm_amap_t *vm_amap_alloc(int slots) {
  slots += EXTRA_AMAP_SLOTS;
  vm_amap_t *amap = pool_alloc(P_VM_AMAP_STRUCT, M_WAITOK);
  if (!amap)
    return NULL;

  amap->pg_list =
    kmalloc(M_AMAP, slots * sizeof(vm_page_t *), M_ZERO | M_WAITOK);

  if (!amap->pg_list)
    goto list_fail;

  amap->pg_bitmap =
    kmalloc(M_AMAP, slots * sizeof(bitstr_t), M_ZERO | M_WAITOK);
  if (!amap->pg_bitmap)
    goto bitmap_fail;

  amap->ref_cnt = 1;
  amap->slots = slots;
  return amap;

bitmap_fail:
  kfree(M_AMAP, amap->pg_list);
list_fail:
  pool_free(P_VM_AMAP_STRUCT, amap);
  return NULL;
}

vm_amap_t *vm_amap_clone(vm_aref_t aref) {
  vm_amap_t *amap = aref.amap;
  if (!amap)
    return NULL;

  vm_amap_t *new = vm_amap_alloc(amap->slots - aref.offset);
  if (!new)
    return NULL;

  SCOPED_MTX_LOCK(&amap->mtx);
  for (int slot = 0; slot < amap->slots - aref.offset; slot++) {

    if (!bit_test(amap->pg_bitmap, aref.offset + slot))
      continue;

    vm_page_t *new_pg = vm_page_alloc_zero(1);
    pmap_copy_page(amap->pg_list[aref.offset + slot], new_pg);

    new->pg_list[slot] = new_pg;
    bit_set(new->pg_bitmap, slot);
  }
  return new;
}

void vm_amap_hold(vm_amap_t *amap) {
  refcnt_acquire(&amap->ref_cnt);
}

void vm_amap_drop(vm_amap_t *amap) {
  if (refcnt_release(&amap->ref_cnt))
    vm_amap_free(amap);
}

vm_page_t *vm_amap_find_page(vm_aref_t aref, int offset) {
  vm_amap_t *amap = aref.amap;
  assert(amap != NULL);

  SCOPED_MTX_LOCK(&amap->mtx);
  if (bit_test(amap->pg_bitmap, offset))
    return amap->pg_list[offset];
  return NULL;
}

int vm_amap_add_page(vm_aref_t aref, vm_page_t *frame, int offset) {
  vm_amap_t *amap = aref.amap;
  assert(amap != NULL && frame != NULL);

  SCOPED_MTX_LOCK(&amap->mtx);
  if (bit_test(amap->pg_bitmap, offset)) {
    return EINVAL;
  }
  amap->pg_list[offset] = frame;
  bit_set(amap->pg_bitmap, offset);
  return 0;
}

static void _vm_amap_remove_pages(vm_amap_t *amap, int start, int nslots) {
  SCOPED_MTX_LOCK(&amap->mtx);
  for (int i = start; i < start + nslots; i++) {
    if (!bit_test(amap->pg_bitmap, i))
      continue;
    vm_page_free(amap->pg_list[i]);
    bit_clear(amap->pg_bitmap, i);
  }
}

void vm_amap_remove_pages(vm_aref_t aref, int start, int nslots) {
  if (!aref.amap)
    return;
  _vm_amap_remove_pages(aref.amap, aref.offset + start, nslots);
}

static void vm_amap_free(vm_amap_t *amap) {
  _vm_amap_remove_pages(amap, 0, amap->slots);
  kfree(M_AMAP, amap->pg_list);
  kfree(M_AMAP, amap->pg_bitmap);
  pool_free(P_VM_AMAP_STRUCT, amap);
}
