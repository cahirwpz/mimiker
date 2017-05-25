#include <klog.h>
#include <mips/tlb.h>
#include <pmap.h>
#include <physmem.h>
#include <vm.h>
#include <ktest.h>

#define TEST_REF_MOD_PAGES_NO 16
/* prime less than pages_no */
#define TEST_REF_MOD_STEP_SIZE 7
#define TEST_REF_MOD_STORE 2
#define TEST_REF_MOD_LOAD 1
#define TEST_REF_MOD_NONE 0

static int test_ref_mod_action[TEST_REF_MOD_PAGES_NO] = {
  TEST_REF_MOD_STORE,
  TEST_REF_MOD_NONE,
  TEST_REF_MOD_NONE,
  TEST_REF_MOD_LOAD,
  TEST_REF_MOD_LOAD,
  TEST_REF_MOD_STORE,
  TEST_REF_MOD_NONE,
  TEST_REF_MOD_NONE,
  TEST_REF_MOD_NONE,
  TEST_REF_MOD_LOAD,
  TEST_REF_MOD_NONE,
  TEST_REF_MOD_STORE,
  TEST_REF_MOD_STORE,
  TEST_REF_MOD_LOAD,
  TEST_REF_MOD_STORE,
  TEST_REF_MOD_STORE
};

static int test_ref_mod_additional_store_idx = 13;

static int referenced_modified_test() {

  vm_addr_t start = 0x01000000 - (TEST_REF_MOD_PAGES_NO / 2) * PAGESIZE;
  vm_addr_t end = start + TEST_REF_MOD_PAGES_NO * PAGESIZE;

  pmap_t *pmap = pmap_new();

  vm_page_t *pg = pm_alloc(TEST_REF_MOD_PAGES_NO);

  pmap_activate(pmap);
  pmap_map(pmap, start, end, pg->paddr, VM_PROT_READ | VM_PROT_WRITE);

  int *ptr;
  int i;

  for (i = 0; i < TEST_REF_MOD_PAGES_NO; i++) {
    int idx = i * TEST_REF_MOD_STEP_SIZE % TEST_REF_MOD_PAGES_NO;
    ptr = (int *)(start + idx * PAGESIZE);
    int temp;

    switch (test_ref_mod_action[idx]) {

      case TEST_REF_MOD_NONE:
        break;

      case TEST_REF_MOD_LOAD:
        temp = *ptr;
        klog("Loading %d from address: 0x%p", temp, ptr);
        break;

      case TEST_REF_MOD_STORE:
        *ptr = 0xdeadbeef;
        klog("Storing at address: 0x%p", ptr);
        break;
    }
  }

  ptr = (int *)(start + test_ref_mod_additional_store_idx * PAGESIZE);
  *ptr = 0xdeadbeef;

  pmap_unmap(pmap, start, end);
  pm_free(pg);
  pmap_delete(pmap);

  klog("Test passed");

  return 0;
}

KTEST_ADD(ref_mod, referenced_modified_test, 0);
