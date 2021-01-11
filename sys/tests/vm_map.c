#define KL_LOG KL_TEST
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/vm_pager.h>
#include <sys/vm_object.h>
#include <sys/vm_map.h>
#include <sys/errno.h>
#include <sys/thread.h>
#include <sys/ktest.h>
#include <sys/sched.h>

static int paging_on_demand_and_memory_protection_demo(void) {
  /* This test mustn't be preempted since PCPU's user-space vm_map will not be
   * restored while switching back. */
  SCOPED_NO_PREEMPTION();

  vm_map_t *orig = vm_map_user();
  vm_map_activate(vm_map_new());

  vm_map_t *kmap = vm_map_kernel();
  vm_map_t *umap = vm_map_user();

  klog("Kernel physical map : %08lx-%08lx", vm_map_start(kmap),
       vm_map_end(kmap));
  klog("User physical map   : %08lx-%08lx", vm_map_start(umap),
       vm_map_end(umap));

  vaddr_t pre_start = 0x1000000;
  vaddr_t start = 0x1001000;
  vaddr_t end = 0x1003000;
  vaddr_t post_end = 0x1004000;
  int n;

  /* preceding redzone segment */
  {
    vm_object_t *obj = vm_object_alloc(VM_DUMMY);
    vm_segment_t *seg =
      vm_segment_alloc(obj, pre_start, start, VM_PROT_NONE, VM_SEG_PRIVATE);
    n = vm_map_insert(umap, seg, VM_FIXED);
    assert(n == 0);
  }

  /* data segment */
  {
    vm_object_t *obj = vm_object_alloc(VM_ANONYMOUS);
    vm_segment_t *seg = vm_segment_alloc(
      obj, start, end, VM_PROT_READ | VM_PROT_WRITE, VM_SEG_PRIVATE);
    n = vm_map_insert(umap, seg, VM_FIXED);
    assert(n == 0);
  }

  /* succeeding redzone segment */
  {
    vm_object_t *obj = vm_object_alloc(VM_DUMMY);
    vm_segment_t *seg =
      vm_segment_alloc(obj, end, post_end, VM_PROT_NONE, VM_SEG_PRIVATE);
    n = vm_map_insert(umap, seg, VM_FIXED);
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

  vm_map_delete(umap);

  /* Restore original vm_map */
  vm_map_activate(orig);

  return KTEST_SUCCESS;
}

static int findspace_demo(void) {
  /* This test mustn't be preempted since PCPU's user-space vm_map will not be
   * restored while switching back. */
  SCOPED_NO_PREEMPTION();

  vm_map_t *orig = vm_map_user();

  vm_map_t *umap = vm_map_new();
  vm_map_activate(umap);

  const vaddr_t addr0 = 0x00400000;
  const vaddr_t addr1 = 0x10000000;
  const vaddr_t addr2 = 0x30000000;
  const vaddr_t addr3 = 0x30005000;
  const vaddr_t addr4 = 0x60000000;

  vm_segment_t *seg;
  vaddr_t t;
  int n;

  seg = vm_segment_alloc(NULL, addr1, addr2, VM_PROT_NONE, VM_SEG_PRIVATE);
  n = vm_map_insert(umap, seg, VM_FIXED);
  assert(n == 0);

  seg = vm_segment_alloc(NULL, addr3, addr4, VM_PROT_NONE, VM_SEG_PRIVATE);
  n = vm_map_insert(umap, seg, VM_FIXED);
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
  seg =
    vm_segment_alloc(NULL, addr2, addr2 + 0x5000, VM_PROT_NONE, VM_SEG_PRIVATE);
  n = vm_map_insert(umap, seg, VM_FIXED);
  assert(n == 0);

  t = addr1;
  n = vm_map_findspace(umap, &t, 0x5000);
  assert(n == 0 && t == addr4);

  t = addr4;
  n = vm_map_findspace(umap, &t, 0x6000);
  assert(n == 0 && t == addr4);

  t = 0;
  n = vm_map_findspace(umap, &t, 0x40000000);
  assert(n == ENOMEM);

  vm_map_delete(umap);

  /* Restore original vm_map */
  vm_map_activate(orig);

  return KTEST_SUCCESS;
}

static int vm_map_insert_fixed(void) {
  /* This test mustn't be preempted since PCPU's user-space vm_map will not be
   * restored while switching back. */
  SCOPED_NO_PREEMPTION();

  vm_map_t *orig = vm_map_user();

  vm_map_t *umap = vm_map_new();
  vm_map_activate(umap);

  const vaddr_t addr0 = 0x10000000;
  const vaddr_t addr1 = 0x10001000;
  const vaddr_t addr2 = 0x10005000;
  const vaddr_t addr3 = 0x10009000;

  vm_segment_t *seg;
  vaddr_t t;
  int n;

  seg = vm_segment_alloc(NULL, addr0, addr1, VM_PROT_NONE, VM_SEG_PRIVATE);
  n = vm_map_insert(umap, seg, VM_FIXED);
  assert(n == 0);

  seg = vm_segment_alloc(NULL, addr2, addr3, VM_PROT_NONE, VM_SEG_PRIVATE);
  n = vm_map_insert(umap, seg, VM_FIXED);
  assert(n == 0);

  t = addr1;
  n = vm_map_findspace(umap, &t, 4 * PAGESIZE);
  assert(n == 0 && t == addr1);

  t = addr1;
  n = vm_map_findspace(umap, &t, 5 * PAGESIZE);
  assert(n == 0 && t == addr3);

  /* Delete first and second mapping and replace them by new one */
  seg = vm_segment_alloc(NULL, addr1, addr1 + 5 * PAGESIZE, VM_PROT_NONE,
                         VM_SEG_PRIVATE);
  n = vm_map_insert(umap, seg, VM_FIXED);
  assert(n == 0);

  t = addr1;
  n = vm_map_findspace(umap, &t, 4 * PAGESIZE);
  assert(n == 0 && t == addr1 + 5 * PAGESIZE);

  vm_map_delete(umap);

  /* Restore original vm_map */
  vm_map_activate(orig);
  return KTEST_SUCCESS;
}

static int vm_map_fixed_bad(void) {
  /* This test mustn't be preempted since PCPU's user-space vm_map will not be
   * restored while switching back. */
  SCOPED_NO_PREEMPTION();

  vm_map_t *orig = vm_map_user();

  vm_map_t *umap = vm_map_new();
  vm_map_activate(umap);

  const vaddr_t addr0 = 0x10000000;
  const vaddr_t addr1 = 0x10006000;
  const vaddr_t addr2 = 0x10007000;
  const vaddr_t addr3 = 0x10010000;

  vm_segment_t *seg;
  vaddr_t t;
  int n;

  seg = vm_segment_alloc(NULL, addr0, addr1, VM_PROT_NONE, VM_SEG_PRIVATE);
  n = vm_map_insert(umap, seg, VM_FIXED);
  assert(n == 0);

  seg = vm_segment_alloc(NULL, addr2, addr3, VM_PROT_NONE, VM_SEG_PRIVATE);
  n = vm_map_insert(umap, seg, VM_FIXED);
  assert(n == 0);

  t = addr1;
  n = vm_map_findspace(umap, &t, PAGESIZE);
  assert(n == 0 && t == addr1);

  t = addr1;
  n = vm_map_findspace(umap, &t, 9 * PAGESIZE);
  assert(n == 0 && t == addr3);

  t = addr1;
  seg = vm_segment_alloc(NULL, addr3, addr3 + 0x90000000, VM_PROT_NONE,
                         VM_SEG_PRIVATE);
  n = vm_map_insert(umap, seg, VM_FIXED);
  assert(n == ENOMEM);

  vm_map_delete(umap);

  /* Restore original vm_map */
  vm_map_activate(orig);
  return KTEST_SUCCESS;
}

KTEST_ADD(vm, paging_on_demand_and_memory_protection_demo, 0);
KTEST_ADD(findspace, findspace_demo, 0);
KTEST_ADD(vm_map_insert_fixed, vm_map_insert_fixed, 0);
KTEST_ADD(vm_map_fixed_bad, vm_map_fixed_bad, 0);
