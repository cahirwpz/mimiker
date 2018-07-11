#include <pmap.h>
#include <physmem.h>
#include <vm.h>
#include <ktest.h>

#define PAGES 16

static int test_kernel_pmap(void) {
  pmap_t *pmap = get_kernel_pmap();

  vm_page_t *pg = pm_alloc(PAGES);
  size_t size = pg->size * PAGESIZE;

  vaddr_t vaddr = pmap->start;
  vaddr_t end = pmap->start + size;

  pmap_enter(pmap, vaddr, end, pg->paddr, VM_PROT_READ | VM_PROT_WRITE);

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

  pm_free(pg);

  return KTEST_SUCCESS;
}

static int test_user_pmap(void) {
  pmap_t *orig = get_user_pmap();

  pmap_t *pmap1 = pmap_new();
  pmap_t *pmap2 = pmap_new();

  vaddr_t start = 0x1001000;
  vaddr_t end = 0x1002000;

  vm_page_t *pg1 = pm_alloc(1);
  vm_page_t *pg2 = pm_alloc(1);

  pmap_activate(pmap1);
  pmap_enter(pmap1, start, end, pg1->paddr, VM_PROT_READ | VM_PROT_WRITE);
  pmap_activate(pmap2);
  pmap_enter(pmap2, start, end, pg2->paddr, VM_PROT_READ | VM_PROT_WRITE);

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

KTEST_ADD(pmap_kernel, test_kernel_pmap, 0);
KTEST_ADD(pmap_user, test_user_pmap, 0);
