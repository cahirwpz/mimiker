#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/uvm_amap.h>
#include <machine/vm_param.h>
#include <sys/ktest.h>

/* Test internal state of amap. */
static int test_amap_simple(void) {
  uvm_amap_t *amap = uvm_amap_alloc();
  assert(amap != NULL);

  uvm_aref_t aref = {.ar_pageoff = 0, .ar_amap = amap};

  uvm_amap_add(&aref, (uvm_anon_t *)1, 3 * PAGESIZE);
  uvm_amap_add(&aref, (uvm_anon_t *)2, 7 * PAGESIZE);
  uvm_amap_add(&aref, (uvm_anon_t *)3, 2 * PAGESIZE);
  uvm_amap_add(&aref, (uvm_anon_t *)4, 14 * PAGESIZE);

  uvm_amap_lock(amap);
  assert(amap->am_nslot == 16);
  assert(amap->am_nused == 4);

  assert(amap->am_anon[3] == (uvm_anon_t *)1);
  assert(amap->am_anon[7] == (uvm_anon_t *)2);
  assert(amap->am_anon[2] == (uvm_anon_t *)3);
  assert(amap->am_anon[14] == (uvm_anon_t *)4);

  assert(amap->am_bckptr[0] == 3);
  assert(amap->am_bckptr[1] == 7);
  assert(amap->am_bckptr[2] == 2);
  assert(amap->am_bckptr[3] == 14);

  assert(amap->am_slot[3] == 0);
  assert(amap->am_slot[7] == 1);
  assert(amap->am_slot[2] == 2);
  assert(amap->am_slot[14] == 3);
  uvm_amap_unlock(amap);

  assert(uvm_amap_lookup(&aref, 7 * PAGESIZE) == (uvm_anon_t *)2);
  uvm_amap_remove(&aref, 7 * PAGESIZE);
  assert(amap->am_nused == 3);
  assert(uvm_amap_lookup(&aref, 7 * PAGESIZE) == NULL);

  uvm_amap_lock(amap);
  assert(amap->am_slot[7] == -1);
  assert(amap->am_bckptr[1] == 14);
  assert(amap->am_bckptr[3] == -1);
  assert(amap->am_anon[7] == NULL);

  uvm_amap_drop(amap);

  return KTEST_SUCCESS;
}

KTEST_ADD(uvm_amap_simple, test_amap_simple, 0);
