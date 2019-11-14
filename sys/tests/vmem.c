#include <sys/mimiker.h>
#include <sys/ktest.h>
#include <sys/vmem.h>

static int test_vmem(void) {
  vmem_t *vm = vmem_create("test vmem", 0, 0, 4096, VMEM_NOSLEEP);
  assert(vm != NULL);

  int rc;
  vmem_addr_t addr;

  rc = vmem_add(vm, 0, 50, VMEM_NOSLEEP);
  assert(rc == 0);
  rc = vmem_add(vm, 100, 200, VMEM_NOSLEEP);
  assert(rc == 0);

  rc = vmem_alloc(vm, 1000, VMEM_INSTANTFIT, &addr);
  assert(rc != 0);

  rc = vmem_alloc(vm, 100, VMEM_INSTANTFIT, &addr);
  assert(rc == 0 && addr != 0);

  return KTEST_SUCCESS;
}

KTEST_ADD(vmem, test_vmem, 0);
