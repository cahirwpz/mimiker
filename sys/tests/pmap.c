#include <sys/klog.h>
#include <sys/pmap.h>
#include <sys/vm.h>
#include <sys/vm_physmem.h>
#include <sys/ktest.h>
#include <sys/sched.h>
#include <sys/kmem.h>

static vm_page_t *x_vm_page_alloc(size_t npages) {
  vm_page_t *pg = vm_page_alloc(npages);
  assert(pg != NULL);
  return pg;
}

static vaddr_t x_kva_alloc(size_t size) {
  vaddr_t vaddr = kva_alloc(size);
  assert(vaddr != 0);
  return vaddr;
}

/*
 * Kernel physical map tests.
 */

static int test_pmap_kenter(void) {
  vm_page_t *pg = x_vm_page_alloc(1);
  vaddr_t va = x_kva_alloc(PAGESIZE);

  bool done;
  unsigned i, val;
  unsigned *ptr = (unsigned *)va;

  /* read-write */
  pmap_kenter(va, pg->paddr, VM_PROT_READ | VM_PROT_WRITE, 0);
  for (i = 0; i < PAGESIZE / sizeof(unsigned); i++) {
    done = try_store_word(&ptr[i], i);
    assert(done);
  }

  /* read-only */
  pmap_kenter(va, pg->paddr, VM_PROT_READ, 0);
  for (i = 0; i < PAGESIZE / sizeof(unsigned); i++) {
    done = try_load_word(&ptr[i], &val);
    assert(done && val == i);
  }

  done = try_store_word(ptr, 0xDEADC0DE);
  assert(!done);

  /* no access allowed */
  pmap_kenter(va, pg->paddr, 0, 0);

  done = try_load_word(ptr, &val);
  assert(!done);

  done = try_store_word(ptr, 0xDEADC0DE);
  assert(!done);

  pmap_kremove(va, PAGESIZE);
  kva_free(va, PAGESIZE);
  vm_page_free(pg);

  return KTEST_SUCCESS;
}

static int test_pmap_kextract(void) {
  vm_page_t *pg = x_vm_page_alloc(1);
  vaddr_t va = x_kva_alloc(PAGESIZE);
  pmap_kenter(va, pg->paddr, VM_PROT_READ, 0);

  paddr_t pa;
  bool ok = pmap_kextract(va, &pa);
  assert(ok && pa == pg->paddr);

  pmap_kremove(va, PAGESIZE);
  kva_free(va, PAGESIZE);
  vm_page_free(pg);

  return KTEST_SUCCESS;
}

static int test_pmap_page_copy(void) {
  vm_page_t *pg1 = x_vm_page_alloc(1);
  vm_page_t *pg2 = x_vm_page_alloc(2);
  vaddr_t va = x_kva_alloc(PAGESIZE);

  bool done;
  unsigned i, val;
  unsigned *ptr = (unsigned *)va;

  pmap_kenter(va, pg1->paddr, VM_PROT_READ | VM_PROT_WRITE, 0);
  for (i = 0; i < PAGESIZE / sizeof(unsigned); i++) {
    done = try_store_word(&ptr[i], i);
    assert(done);
  }

  pmap_copy_page(pg1, pg2);
  pmap_zero_page(pg1);
  for (i = 0; i < PAGESIZE / sizeof(unsigned); i++) {
    done = try_load_word(&ptr[i], &val);
    assert(done && val == 0);
  }

  pmap_kenter(va, pg2->paddr, VM_PROT_READ, 0);
  for (i = 0; i < PAGESIZE / sizeof(unsigned); i++) {
    done = try_load_word(&ptr[i], &val);
    assert(done && val == i);
  }

  pmap_kremove(va, PAGESIZE);
  kva_free(va, PAGESIZE);
  vm_page_free(pg1);
  vm_page_free(pg2);

  return KTEST_SUCCESS;
}

KTEST_ADD(pmap_kenter, test_pmap_kenter, 0);
KTEST_ADD(pmap_kextract, test_pmap_kextract, 0);
KTEST_ADD(pmap_page_copy, test_pmap_page_copy, 0);

/*
 * User physical map tests.
 */

static int test_user_pmap(void) {
  /* This test mustn't be preempted since PCPU's user-space vm_map
   * (and its pmap) will not be restored while switching back. */
  SCOPED_NO_PREEMPTION();

  pmap_t *orig = pmap_user();

  pmap_t *pmap1 = pmap_new();
  pmap_t *pmap2 = pmap_new();

  vaddr_t start = 0x1001000;

  vm_page_t *pg1 = x_vm_page_alloc(1);
  vm_page_t *pg2 = x_vm_page_alloc(1);

  pmap_activate(pmap1);
  pmap_enter(pmap1, start, pg1, VM_PROT_READ | VM_PROT_WRITE, 0);
  pmap_activate(pmap2);
  pmap_enter(pmap2, start, pg2, VM_PROT_READ | VM_PROT_WRITE, 0);

  volatile int *ptr = (int *)start;
  *ptr = 100;
  pmap_activate(pmap1);
  *ptr = 200;
  pmap_activate(pmap2);
  assert(*ptr == 100);
  pmap_activate(pmap1);
  assert(*ptr == 200);

  pmap_delete(pmap1);
  pmap_delete(pmap2);

  /* Restore original user pmap */
  pmap_activate(orig);

  return KTEST_SUCCESS;
}

static int test_rmbits(void) {
  /* This test mustn't be preempted since PCPU's user-space vm_map
   * (and its pmap) will not be restored while switching back. */
  SCOPED_NO_PREEMPTION();

  pmap_t *orig = pmap_user();

  pmap_t *pmap = pmap_new();

  volatile int *ptr = (int *)0x1001000;

  vm_page_t *pg = x_vm_page_alloc(1);

  pmap_activate(pmap);
  pmap_enter(pmap, (vaddr_t)ptr, pg, VM_PROT_READ | VM_PROT_WRITE, 0);

  assert(!pmap_is_referenced(pg) && !pmap_is_modified(pg));

  /* vm_page_alloc doesn't return zeroed pages, so we cannot assume any value */
  __unused int value = *ptr;

  assert(pmap_is_referenced(pg) && !pmap_is_modified(pg));

  pmap_clear_referenced(pg);

  *ptr = 100;

  assert(pmap_is_referenced(pg) && pmap_is_modified(pg));

  pmap_clear_modified(pg);

  assert(*ptr == 100);

  assert(pmap_is_referenced(pg) && !pmap_is_modified(pg));

  pmap_delete(pmap);

  /* Restore original user pmap */
  pmap_activate(orig);

  return KTEST_SUCCESS;
}

KTEST_ADD(pmap_user, test_user_pmap, 0);
KTEST_ADD(pmap_rmbits, test_rmbits, 0);
