#include <stdc.h>
#include <mips/tlb.h>
#include <pmap.h>
#include <physmem.h>
#include <vm.h>

#if 0
/* BUG: This should work! */
#define BASEADDR 0xc0400000
#else
#define BASEADDR 0xc0800000
#endif

int main() {
  pmap_t *pmap = get_active_pmap(PMAP_KERNEL);

  vm_page_t *pg1 = pm_alloc(4);
  vaddr_t vaddr = BASEADDR + PAGESIZE * 10;
  pmap_map(pmap, vaddr, pg1->paddr, pg1->size, VM_PROT_READ|VM_PROT_WRITE);

  {
    log("TLB before:");
    tlb_print();

    int *x = (int *)vaddr;
    for (int i = 0; i < 1024 * pg1->size; i++)
      *(x + i) = i;
    for (int i = 0; i < 1024 * pg1->size; i++)
      assert(*(x + i) == i);

    log("TLB after:");
    tlb_print();
  }

  vm_page_t *pg2 = pm_alloc(1);
  vaddr = BASEADDR + PAGESIZE * 2000;
  pmap_map(pmap, vaddr, pg2->paddr, pg2->size, VM_PROT_READ|VM_PROT_WRITE);

  {
    log("TLB before:");
    tlb_print();

    int *x = (int *)vaddr;
    for (int i = 0; i < 1024 * pg2->size; i++)
      *(x + i) = i;
    for (int i = 0; i < 1024 * pg2->size; i++)
      assert(*(x + i) == i);

    log("TLB after:");
    tlb_print();
  }

  pm_free(pg1);
  pm_free(pg2);

  pmap_reset(pmap);
  kprintf("Tests passed\n");
  return 0;
}
