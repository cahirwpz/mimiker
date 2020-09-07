#include <sys/mimiker.h>
#include <sys/pmap.h>
#include <sys/vm.h>
#include <sys/vm_physmem.h>
#include <sys/ktest.h>
#include <sys/sched.h>
#include <sys/kmem.h>

#define PAGES 16

static int test_kernel_pmap(void) {
  pmap_t *pmap = pmap_kernel();

  vm_page_t *pg = vm_page_alloc(PAGES);
  size_t size = pg->size * PAGESIZE;

  vaddr_t vaddr = pmap_start(pmap);
  vaddr_t end = vaddr + size;

  pmap_enter(pmap, vaddr, pg, VM_PROT_READ | VM_PROT_WRITE, 0);

  unsigned *array = (void *)vaddr;
  for (unsigned i = 0; i < size / sizeof(int); i++)
    assert(try_store_word(&array[i], i));

  pmap_protect(pmap, vaddr, vaddr + size, VM_PROT_READ);
  for (vaddr_t addr = vaddr; addr < end; addr += PAGESIZE)
    assert(!try_store_word((void *)addr, 0));

  for (unsigned i = 0; i < size / sizeof(int); i++) {
    unsigned val;
    assert(try_load_word(&array[i], &val));
    assert(val == i);
  }

  for (vaddr_t addr = vaddr; addr < end; addr += PAGESIZE) {
    pmap_remove(pmap, addr, addr + PAGESIZE);
    unsigned val;
    assert(!try_load_word((void *)addr, &val));
  }

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

  pmap_t *orig = pmap_user();
  pmap_t *pmap = pmap_kernel();

  pmap_activate(pmap);

  vm_page_t *pg = vm_page_alloc(1);
  vaddr_t va = kva_alloc(PAGESIZE);

  volatile uint64_t *ptr = (uint64_t *)va;

  pmap_kenter(va, pg->paddr, VM_PROT_READ | VM_PROT_WRITE, 0);

#ifndef KASAN
  *ptr = 0xDEADC0DE;
  assert(*ptr == 0xDEADC0DE);
#endif /* !KASAN */

  pmap_kremove(va, va + PAGESIZE);
  kva_free(va, PAGESIZE);
  pmap_activate(orig);

  return KTEST_SUCCESS;
}

KTEST_ADD(pmap_kernel, test_kernel_pmap, KTEST_FLAG_BROKEN);
KTEST_ADD(pmap_user, test_user_pmap, 0);
KTEST_ADD(pmap_rmbits, test_rmbits, 0);
KTEST_ADD(pmap_kenter, test_pmap_kenter, 0);
