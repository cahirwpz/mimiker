#include <sys/mimiker.h>
#include <sys/pmap.h>
#include <sys/vm.h>
#include <sys/vm_physmem.h>
#include <sys/ktest.h>
#include <sys/sched.h>
#include <sys/kmem.h>
#include <sys/kasan.h>

#define PAGES 16

static int test_kernel_pmap(void) {
  pmap_t *pmap = pmap_kernel();

  vm_page_t *pg = vm_page_alloc(PAGES);
  assert(pg);

  size_t size = pg->size * PAGESIZE;

  vaddr_t vaddr = kva_alloc(size);
  assert(vaddr);

  kasan_mark_valid((void *)vaddr, size);

  pmap_enter(pmap, vaddr, pg, VM_PROT_READ | VM_PROT_WRITE, 0);

  unsigned *ptr = (unsigned *)vaddr;
  for (unsigned i = 0; i < size / sizeof(unsigned); i++)
    assert(try_store_word(&ptr[i], i % 123));

  for (unsigned i = 0; i < size / sizeof(unsigned); i++) {
    unsigned val;
    assert(try_load_word(&ptr[i], &val));
    assert(val == i % 123);
  }

  pmap_remove(pmap, vaddr, vaddr + size);

  kasan_mark_invalid((void *)vaddr, size, KASAN_CODE_KMEM_FREED);

  kva_free(vaddr, size);
  vm_page_free(pg);

  return KTEST_SUCCESS;
}

static int test_user_pmap(void) {
  /* This test mustn't be preempted since PCPU's user-space vm_map
   * (and its pmap) will not be restored while switching back. */
  SCOPED_NO_PREEMPTION();

  pmap_t *orig = pmap_user();

  pmap_t *pmap1 = pmap_new();
  pmap_t *pmap2 = pmap_new();

  vaddr_t start = 0x1001000;

  vm_page_t *pg1 = vm_page_alloc(1);
  vm_page_t *pg2 = vm_page_alloc(1);

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

  vm_page_t *pg = vm_page_alloc(1);

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

static int test_pmap_kenter(void) {
  SCOPED_NO_PREEMPTION();

  vm_page_t *pg = vm_page_alloc(1);
  assert(pg);
  vaddr_t va = kva_alloc(PAGESIZE);
  assert(va);

  pmap_kenter(va, pg->paddr, VM_PROT_READ | VM_PROT_WRITE, 0);

  kasan_mark_valid((void *)va, PAGESIZE);

  volatile uint64_t *ptr = (uint64_t *)va;
  *ptr = 0xDEADC0DE;
  assert(*ptr == 0xDEADC0DE);

  kasan_mark_invalid((void *)va, PAGESIZE, KASAN_CODE_KMEM_FREED);

  pmap_kremove(va, va + PAGESIZE);
  kva_free(va, PAGESIZE);
  vm_page_free(pg);

  return KTEST_SUCCESS;
}

static int test_pmap_page(void) {
  SCOPED_NO_PREEMPTION();

  vm_page_t *pg1 = vm_page_alloc(1);
  vm_page_t *pg2 = vm_page_alloc(2);
  assert(pg1 && pg2);

  volatile vaddr_t va = kva_alloc(PAGESIZE);
  assert(va);

  pmap_kenter(va, pg1->paddr, VM_PROT_READ | VM_PROT_WRITE, 0);

  kasan_mark_valid((void *)va, PAGESIZE);

  volatile uint8_t *buf = (uint8_t *)va;
  for (int i = 0; i < PAGESIZE; i++)
    buf[i] = i % 123;

  pmap_copy_page(pg1, pg2);
  pmap_zero_page(pg1);

  for (int i = 0; i < PAGESIZE; i++)
    assert(buf[i] == 0);

  pmap_kremove(va, va + PAGESIZE);

  pmap_kenter(va, pg2->paddr, VM_PROT_READ, 0);

  for (int i = 0; i < PAGESIZE; i++)
    assert(buf[i] == i % 123);

  kasan_mark_invalid((void *)va, PAGESIZE, KASAN_CODE_KMEM_FREED);

  pmap_kremove(va, va + PAGESIZE);
  kva_free(va, PAGESIZE);
  vm_page_free(pg1);
  vm_page_free(pg2);

  return KTEST_SUCCESS;
}

static int test_pmap_kextract(void) {
  SCOPED_NO_PREEMPTION();

  vm_page_t *pg = vm_page_alloc(1);
  assert(pg);

  vaddr_t va = kva_alloc(PAGESIZE);
  assert(va);

  pmap_kenter(va, pg->paddr, VM_PROT_READ, 0);

  paddr_t pa;
  bool rc = pmap_kextract(va, &pa);
  assert(rc);
  assert(pa == pg->paddr);

  pmap_kremove(va, va + PAGESIZE);

  kva_free(va, PAGESIZE);
  vm_page_free(pg);

  return KTEST_SUCCESS;
}

KTEST_ADD(pmap_kernel, test_kernel_pmap, 0);
KTEST_ADD(pmap_user, test_user_pmap, 0);
KTEST_ADD(pmap_rmbits, test_rmbits, 0);
KTEST_ADD(pmap_kenter, test_pmap_kenter, 0);
KTEST_ADD(pmap_page, test_pmap_page, 0);
KTEST_ADD(pmap_kextract, test_pmap_kextract, 0);
