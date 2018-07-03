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

  klog("Kernel physical map : %08lx-%08lx", kmap->pmap->start, kmap->pmap->end);
  klog("User physical map   : %08lx-%08lx", umap->pmap->start, umap->pmap->end);

  vm_addr_t start = 0x1001000;
  vm_addr_t end = 0x1001000 + 2 * PAGESIZE;

  vm_object_t *redzone_before = vm_object_alloc(VM_DUMMY);
  vm_object_t *redzone_after = vm_object_alloc(VM_DUMMY);
  vm_object_t *data = vm_object_alloc(VM_ANONYMOUS);

  (void)vm_map_insert(umap, redzone_before, start - PAGESIZE, start,
                      VM_PROT_NONE);
  (void)vm_map_insert(umap, data, start, end, VM_PROT_READ | VM_PROT_WRITE);
  (void)vm_map_insert(umap, redzone_after, end, end + PAGESIZE, VM_PROT_NONE);

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

#define addr1 0x10000000
#define addr2 0x30000000
  vm_map_insert(umap, NULL, addr1, addr2, VM_PROT_NONE);
#define addr3 0x30005000
#define addr4 0x60000000
  vm_map_insert(umap, NULL, addr3, addr4, VM_PROT_NONE);

  vm_addr_t t;
  int n;
  n = vm_map_findspace(umap, 0x00400000, PAGESIZE, &t);
  assert(n == 0 && t == 0x00400000);

  n = vm_map_findspace(umap, addr1, PAGESIZE, &t);
  assert(n == 0 && t == addr2);

  n = vm_map_findspace(umap, addr1 + 20 * PAGESIZE, PAGESIZE, &t);
  assert(n == 0 && t == addr2);

  n = vm_map_findspace(umap, addr1, 0x6000, &t);
  assert(n == 0 && t == addr4);

  n = vm_map_findspace(umap, addr1, 0x5000, &t);
  assert(n == 0 && t == addr2);

  /* Fill the gap exactly */
  vm_map_insert(umap, NULL, t, t + 0x5000, VM_PROT_NONE);

  n = vm_map_findspace(umap, addr1, 0x5000, &t);
  assert(n == 0 && t == addr4);

  n = vm_map_findspace(umap, addr4, 0x6000, &t);
  assert(n == 0 && t == addr4);

  n = vm_map_findspace(umap, 0, 0x40000000, &t);
  assert(n == -ENOMEM);

  /* Restore original vm_map */
  vm_map_activate(orig);

  vm_map_delete(umap);

  klog("Test passed.");
  return KTEST_SUCCESS;
}

KTEST_ADD(vm, paging_on_demand_and_memory_protection_demo, 0);
KTEST_ADD(findspace, findspace_demo, 0);
