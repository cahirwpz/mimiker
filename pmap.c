#include <stdc.h>
#include <mips/tlb.h>
#include <pmap.h>
#include <physmem.h>
#include <vm.h>


void general_pmap_test() {
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
}

int *vm_addr_to_ptr(vm_addr_t addr) {
    return (int*) addr;
}

static void change_pmap() {
  pmap_t pmap1;
  pmap_t pmap2;
  
  pmap_setup(&pmap1, PMAP_USER);
  pmap_setup(&pmap2, PMAP_USER);
  

  vm_addr_t start = 0x1001000;
  vm_addr_t end = 0x1002000;

  vm_page_t *pg1 = pm_alloc(1);
  vm_page_t *pg2 = pm_alloc(1);

  set_active_pmap(&pmap1);
  pmap_map(&pmap1, start, end, pg1->paddr, VM_PROT_READ | VM_PROT_WRITE);
  set_active_pmap(&pmap2);
  pmap_map(&pmap2, start, end, pg2->paddr, VM_PROT_READ | VM_PROT_WRITE);
  
  volatile int *ptr = vm_addr_to_ptr(start);
  *ptr = 100;
  pmap_switch(&pmap1);
  *ptr = 200;
  pmap_switch(&pmap2);
  assert(*ptr == 100);
  kprintf("*ptr == %d\n", *ptr);
  pmap_switch(&pmap1);
  assert(*ptr == 200);
  kprintf("*ptr == %d\n", *ptr);
}

int main() {
  change_pmap();
  return 0;
}

