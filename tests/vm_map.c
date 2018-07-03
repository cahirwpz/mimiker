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
  int n;

  vm_map_range(kmap, &start, &end);
  klog("Kernel physical map : %08lx-%08lx", start, end);
  vm_map_range(umap, &start, &end);
  klog("User physical map   : %08lx-%08lx", start, end);

  start = 0x1001000;
  end = 0x1001000 + 2 * PAGESIZE;

  /* preceding redzone segment */
  {
    vm_object_t *obj = vm_object_alloc(VM_DUMMY);
    vm_map_entry_t *entry =
      vm_map_entry_alloc(obj, start - PAGESIZE, PAGESIZE, VM_PROT_NONE);
    n = vm_map_insert(umap, entry, VM_FIXED);
    assert(n == 0);
  }

  /* data segment */
  {
    vm_object_t *obj = vm_object_alloc(VM_ANONYMOUS);
    vm_map_entry_t *entry =
      vm_map_entry_alloc(obj, start, end - start, VM_PROT_READ | VM_PROT_WRITE);
    n = vm_map_insert(umap, entry, VM_FIXED);
    assert(n == 0);
  }

  /* succeeding redzone segment */
  {
    vm_object_t *obj = vm_object_alloc(VM_DUMMY);
    vm_map_entry_t *entry =
      vm_map_entry_alloc(obj, end, PAGESIZE, VM_PROT_NONE);
    n = vm_map_insert(umap, entry, VM_FIXED);
    assert(n == 0);
  }

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

  vm_map_entry_t *entry;
  vm_addr_t t;
  int n;

  entry = vm_map_entry_alloc(NULL, addr1, addr2 - addr1, VM_PROT_NONE);
  n = vm_map_insert(umap, entry, VM_FIXED);
  assert(n == 0);

  entry = vm_map_entry_alloc(NULL, addr3, addr4 - addr3, VM_PROT_NONE);
  n = vm_map_insert(umap, entry, VM_FIXED);
  assert(n == 0);

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
  entry = vm_map_entry_alloc(NULL, addr2, 0x5000, VM_PROT_NONE);
  n = vm_map_insert(umap, entry, VM_FIXED);
  assert(n == 0);

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
