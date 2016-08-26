#include <stdc.h>
#include <mips/tlb.h>
#include <pmap.h>
#include <physmem.h>
#include <vm.h>

int main() {
  pmap_t *pmap = get_active_pmap(PMAP_KERNEL);

  vm_page_t *pg = pm_alloc(16);
  size_t size = pg->size * PAGESIZE;
  vm_addr_t vaddr1 = pmap->start;
  vm_addr_t vaddr2 = pmap->start + size / 2;
  vm_addr_t vaddr3 = pmap->start + size;
  pmap_map(pmap, vaddr1, vaddr3, pg->paddr, VM_PROT_READ|VM_PROT_WRITE);

  {
    log("TLB before:");
    tlb_print();

    int *x = (int *)vaddr1;
    for (int i = 0; i < size / sizeof(int); i++)
      *(x + i) = i;
    for (int i = 0; i < size / sizeof(int); i++)
      assert(*(x + i) == i);

    log("TLB after:");
    tlb_print();
  }

  assert(pmap_probe(pmap, vaddr1, vaddr3, VM_PROT_READ|VM_PROT_WRITE));

  pmap_unmap(pmap, vaddr1, vaddr2);
  pmap_protect(pmap, vaddr2, vaddr3, VM_PROT_READ);

  assert(pmap_probe(pmap, vaddr1, vaddr2, VM_PROT_NONE));
  assert(pmap_probe(pmap, vaddr2, vaddr3, VM_PROT_READ));

  pmap_reset(pmap);
  pm_free(pg);

  kprintf("Tests passed\n");
  return 0;
}
