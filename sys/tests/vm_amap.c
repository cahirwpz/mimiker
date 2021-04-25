#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/vm_amap.h>
#include <sys/vm_anon.h>
#include <machine/vm_param.h>
#include <sys/ktest.h>

/* Test internal state of amap. */
static int test_amap_simple(void) {
  vm_amap_t *amap = vm_amap_alloc();
  assert(amap != NULL);

  vm_aref_t aref = {.ar_pageoff = 0, .ar_amap = amap};
  vm_amap_lock(amap);
  vm_amap_hold(amap);
  vm_amap_unlock(amap);

  vm_anon_t *an1 = vm_anon_alloc();
  vm_anon_t *an2 = vm_anon_alloc();
  vm_anon_t *an3 = vm_anon_alloc();
  vm_anon_t *an4 = vm_anon_alloc();

  vm_amap_add(&aref, an1, 3 * PAGESIZE);
  vm_amap_add(&aref, an2, 7 * PAGESIZE);
  vm_amap_add(&aref, an3, 2 * PAGESIZE);
  vm_amap_add(&aref, an4, 14 * PAGESIZE);

  vm_amap_lock(amap);
  assert(amap->am_nslot == 16);
  assert(amap->am_nused == 4);

  assert(amap->am_anon[3] == an1);
  assert(amap->am_anon[7] == an2);
  assert(amap->am_anon[2] == an3);
  assert(amap->am_anon[14] == an4);

  assert(amap->am_bckptr[0] == 3);
  assert(amap->am_bckptr[1] == 7);
  assert(amap->am_bckptr[2] == 2);
  assert(amap->am_bckptr[3] == 14);

  assert(amap->am_slot[3] == 0);
  assert(amap->am_slot[7] == 1);
  assert(amap->am_slot[2] == 2);
  assert(amap->am_slot[14] == 3);
  vm_amap_unlock(amap);

  assert(vm_amap_lookup(&aref, 7 * PAGESIZE) == an2);
  vm_amap_remove(&aref, 7 * PAGESIZE);
  assert(amap->am_nused == 3);
  assert(vm_amap_lookup(&aref, 7 * PAGESIZE) == NULL);

  vm_amap_lock(amap);
  assert(amap->am_slot[7] == -1);
  assert(amap->am_bckptr[1] == 14);
  assert(amap->am_bckptr[3] == -1);
  assert(amap->am_anon[7] == NULL);
  vm_amap_unlock(amap);

  vm_amap_lock(amap);
  assert(amap->am_ref == 1);
  vm_amap_drop(amap);

  return KTEST_SUCCESS;
}

static int test_amap_split(void) {
  vm_amap_t *amap = vm_amap_alloc();
  assert(amap != NULL);

  vm_aref_t aref = {.ar_pageoff = 0, .ar_amap = amap};

  vm_amap_lock(amap);
  vm_amap_hold(amap);
  vm_amap_unlock(amap);

  vm_anon_t *an1 = vm_anon_alloc();
  vm_anon_t *an2 = vm_anon_alloc();
  vm_anon_t *an3 = vm_anon_alloc();
  vm_anon_t *an4 = vm_anon_alloc();

  vm_amap_add(&aref, an1, 3 * PAGESIZE);
  vm_amap_add(&aref, an2, 7 * PAGESIZE);
  vm_amap_add(&aref, an3, 2 * PAGESIZE);
  vm_amap_add(&aref, an4, 14 * PAGESIZE);

  vm_aref_t new_aref = vm_amap_split(&aref, 5 * PAGESIZE);
  vm_amap_lock(amap);
  assert(amap->am_ref == 2);
  vm_amap_unlock(amap);

  vm_anon_t *an5 = vm_anon_alloc();
  vm_anon_t *an6 = vm_anon_alloc();

  vm_amap_add(&aref, an5, 1 * PAGESIZE);
  vm_amap_add(&new_aref, an6, 1 * PAGESIZE);

  vm_amap_lock(amap);
  assert(amap->am_nused == 6);
  assert(amap->am_anon[1] == an5);
  assert(amap->am_anon[6] == an6);
  assert(amap->am_bckptr[4] == 1);
  assert(amap->am_bckptr[5] == 6);
  vm_amap_unlock(amap);

  assert(vm_amap_lookup(&aref, 1 * PAGESIZE) == an5);
  assert(vm_amap_lookup(&new_aref, 1 * PAGESIZE) == an6);
  assert(vm_amap_lookup(&new_aref, (14 - 5) * PAGESIZE) == an4);

  /* we have to drop amap twice becouse we have two refs now */
  vm_amap_lock(amap);
  vm_amap_drop(amap);

  vm_amap_lock(amap);
  assert(amap->am_ref == 1);
  vm_amap_drop(amap);

  return KTEST_SUCCESS;
}

KTEST_ADD(vm_amap_simple, test_amap_simple, 0);
KTEST_ADD(vm_amap_split, test_amap_split, 0);
