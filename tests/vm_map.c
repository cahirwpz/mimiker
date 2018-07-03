#define KL_LOG KL_TEST
#include <klog.h>
#include <stdc.h>
#include <vm_pager.h>
#include <vm_object.h>
#include <vm_map.h>
#include <errno.h>
#include <thread.h>
#include <ktest.h>

static int paging_on_demand_and_memory_protection_demo(void) {
  vm_map_t *orig = get_user_vm_map();
  vm_map_activate(vm_map_new());

  vm_map_t *kmap = get_kernel_vm_map();
  vm_map_t *umap = get_user_vm_map();

  vm_addr_t start, end;

  vm_map_range(kmap, &start, &end);
  klog("Kernel physical map : %08lx-%08lx", start, end);
  vm_map_range(umap, &start, &end);
  klog("User physical map   : %08lx-%08lx", start, end);

  start = 0x1001000;
  end = 0x1001000 + 2 * PAGESIZE;

  vm_object_t *redzone_before = vm_object_alloc(VM_DUMMY);
  vm_object_t *redzone_after = vm_object_alloc(VM_DUMMY);
  vm_object_t *data = vm_object_alloc(VM_ANONYMOUS);

  (void)vm_map_insert(umap, redzone_before, start - PAGESIZE, PAGESIZE,
                      VM_PROT_NONE);
  (void)vm_map_insert(umap, data, start, end - start, VM_PROT_READ | VM_PROT_WRITE);
  (void)vm_map_insert(umap, redzone_after, end, PAGESIZE, VM_PROT_NONE);

  vm_map_dump(umap);
  vm_map_dump(kmap);

  /* Start in paged on demand range, but end outside, to cause fault */
  for (int *ptr = (int *)start; ptr != (int *)end; ptr += 256) {
    klog("%p", ptr);
    *ptr = 0xfeedbabe;
  }

  vm_map_dump(umap);
  vm_map_dump(kmap);

  /* Restore original vm_map */
  vm_map_activate(orig);

  vm_map_delete(umap);

  klog("Test passed.");
  return KTEST_SUCCESS;
}

static int findspace_demo(void) {
  vm_map_t *orig = get_user_vm_map();

  vm_map_t *umap = vm_map_new();
  vm_map_activate(umap);

  const vm_addr_t addr0 = 0x00400000;
  const vm_addr_t addr1 = 0x10000000;
  const vm_addr_t addr2 = 0x30000000;
  const vm_addr_t addr3 = 0x30005000;
  const vm_addr_t addr4 = 0x60000000;

  vm_map_insert(umap, NULL, addr1, addr2 - addr1, VM_PROT_NONE);
  vm_map_insert(umap, NULL, addr3, addr4 - addr3, VM_PROT_NONE);

  vm_addr_t t;
  int n;

  t = addr0;
  n = vm_map_findspace(umap, &t, PAGESIZE);
  assert(n == 0 && t == addr0);

  t = addr1;
  n = vm_map_findspace(umap, &t, PAGESIZE);
  assert(n == 0 && t == addr2);

  t = addr1 + 20 * PAGESIZE;
  n = vm_map_findspace(umap, &t, PAGESIZE);
  assert(n == 0 && t == addr2);

  t = addr1;
  n = vm_map_findspace(umap, &t, 0x6000);
  assert(n == 0 && t == addr4);

  t = addr1;
  n = vm_map_findspace(umap, &t, 0x5000);
  assert(n == 0 && t == addr2);

  /* Fill the gap exactly */
  vm_map_insert(umap, NULL, t, 0x5000, VM_PROT_NONE);

  t = addr1;
  n = vm_map_findspace(umap, &t, 0x5000);
  assert(n == 0 && t == addr4);

  t = addr4;
  n = vm_map_findspace(umap, &t, 0x6000);
  assert(n == 0 && t == addr4);

  t = 0;
  n = vm_map_findspace(umap, &t, 0x40000000);
  assert(n == -ENOMEM);

  /* Restore original vm_map */
  vm_map_activate(orig);

  vm_map_delete(umap);

  klog("Test passed.");
  return KTEST_SUCCESS;
}

KTEST_ADD(vm, paging_on_demand_and_memory_protection_demo, 0);
KTEST_ADD(findspace, findspace_demo, 0);
