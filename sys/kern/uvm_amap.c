#define KL_LOG KL_VM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/pool.h>
#include <sys/malloc.h>
#include <sys/uvm_amap.h>

#define AMAP_SLOT_EMPTY (-1)

static POOL_DEFINE(P_AMAP, "uvm_amap", sizeof(uvm_amap_t));

static inline int uvm_amap_slot(uvm_aref_t *aref, vaddr_t offset) {
  assert(offset % PAGESIZE == 0);
  return (int)(aref->ar_pageoff + (offset >> PAGE_SHIFT));
}

uvm_amap_t *uvm_amap_alloc(void) {
  uvm_amap_t *amap = pool_alloc(P_AMAP, M_ZERO);
  mtx_init(&amap->am_lock, 0);
  return amap;
}

void uvm_amap_lock(uvm_amap_t *amap) {
  mtx_lock(&amap->am_lock);
}

void uvm_amap_unlock(uvm_amap_t *amap) {
  mtx_unlock(&amap->am_lock);
}

void uvm_amap_hold(uvm_amap_t *amap) {
  assert(mtx_owned(&amap->am_lock));
  amap->am_ref++;
}

/* TODO: revisit after uvm_anon implementation. */
static void uvm_anon_free(uvm_anon_t *anon) {
  assert(anon != NULL);
}

static void uvm_amap_free(uvm_amap_t *amap) {
  for (int i = 0; i < amap->am_nused; ++i) {
    int slot = amap->am_bckptr[i];
    uvm_anon_free(amap->am_anon[slot]);
  }

  kfree(M_TEMP, amap->am_slot);
  kfree(M_TEMP, amap->am_bckptr);
  kfree(M_TEMP, amap->am_anon);

  pool_free(P_AMAP, amap);
}

void uvm_amap_drop(uvm_amap_t *amap) {
  assert(mtx_owned(&amap->am_lock));
  amap->am_ref--;

  uvm_amap_unlock(amap);
  if (amap->am_ref == 0)
    uvm_amap_free(amap);
}

static uvm_anon_t *uvm_amap_lookup_nolock(uvm_amap_t *amap, int slot) {
  if (amap->am_nslot <= slot)
    return NULL;
  return amap->am_anon[slot];
}

uvm_anon_t *uvm_amap_lookup(uvm_aref_t *aref, vaddr_t offset) {
  uvm_amap_t *amap = aref->ar_amap;
  int slot = uvm_amap_slot(aref, offset);

  SCOPED_MTX_LOCK(&amap->am_lock);
  return uvm_amap_lookup_nolock(amap, slot);
}

static void __uvm_amap_extend(uvm_amap_t *amap, int size) {
  /* new entries in slot & bckptr will be set to AMAP_SLOT_EMPTY */
  int *slot = kmalloc(M_TEMP, sizeof(int) * size, 0);
  int *bckptr = kmalloc(M_TEMP, sizeof(int) * size, 0);
  /* new entries in anon will be set to NULL */
  uvm_anon_t **anon = kmalloc(M_TEMP, sizeof(uvm_anon_t *) * size, M_ZERO);

  /* old unused entires will be set to AMAP_SLOT_EMPTY */
  memcpy(slot, amap->am_slot, sizeof(int) * amap->am_nslot);
  memcpy(bckptr, amap->am_bckptr, sizeof(int) * amap->am_nslot);

  /* copy pointers only for allocated anons */
  for (int i = 0; i < amap->am_nused; ++i) {
    int bslot = amap->am_bckptr[i];
    anon[bslot] = amap->am_anon[bslot];
  }

  /* set new entries to AMAP_SLOT_EMPTY */
  for (int i = amap->am_nslot; i < size; ++i) {
    slot[i] = AMAP_SLOT_EMPTY;
    bckptr[i] = AMAP_SLOT_EMPTY;
  }

  kfree(M_TEMP, amap->am_slot);
  kfree(M_TEMP, amap->am_bckptr);
  kfree(M_TEMP, amap->am_anon);

  amap->am_slot = slot;
  amap->am_bckptr = bckptr;
  amap->am_anon = anon;

  amap->am_nslot = size;
}

static void uvm_amap_extend(uvm_amap_t *amap, int size) {
  int capacity = max(amap->am_nslot, 1);
  while (capacity < size)
    capacity *= 2;
  __uvm_amap_extend(amap, capacity);
}

static void uvm_amap_add_nolock(uvm_amap_t *amap, uvm_anon_t *anon, int slot) {
  if (slot >= amap->am_nslot)
    uvm_amap_extend(amap, slot);

  assert(amap->am_anon[slot] == NULL);
  amap->am_anon[slot] = anon;
  amap->am_slot[slot] = amap->am_nused;
  amap->am_bckptr[amap->am_nused++] = slot;
}

void uvm_amap_add(uvm_aref_t *aref, uvm_anon_t *anon, vaddr_t offset) {
  uvm_amap_t *amap = aref->ar_amap;
  int slot = uvm_amap_slot(aref, offset);

  SCOPED_MTX_LOCK(&amap->am_lock);
  uvm_amap_add_nolock(amap, anon, slot);
}

static void uvm_amap_remove_nolock(uvm_amap_t *amap, int slot) {
  assert(amap->am_nslot > slot);
  assert(amap->am_anon[slot] != NULL);

  uvm_anon_free(amap->am_anon[slot]);
  amap->am_anon[slot] = NULL;

  int bslot = amap->am_slot[slot];
  amap->am_slot[slot] = AMAP_SLOT_EMPTY;
  amap->am_bckptr[bslot] = AMAP_SLOT_EMPTY;
  swap(amap->am_bckptr[bslot], amap->am_bckptr[amap->am_nused - 1]);
  amap->am_nused--;
}

void uvm_amap_remove(uvm_aref_t *aref, vaddr_t offset) {
  uvm_amap_t *amap = aref->ar_amap;
  int slot = uvm_amap_slot(aref, offset);

  SCOPED_MTX_LOCK(&amap->am_lock);
  uvm_amap_remove_nolock(amap, slot);
}
