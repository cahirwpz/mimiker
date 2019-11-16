#include <sys/mimiker.h>
#include <sys/ktest.h>
#include <sys/vmem.h>
#include <sys/errno.h>

typedef struct span {
  vmem_addr_t addr;
  vmem_size_t size;
} span_t;

static void assert_addr_is_in_span(vmem_addr_t addr, vmem_size_t size,
                                   span_t *span) {
  assert(addr >= span->addr && addr + size <= span->addr + span->size);
}

static int test_vmem(void) {
  int quantum = 1 << 12;
  vmem_t *vm = vmem_create("test vmem", 0, 0, quantum, VMEM_NOSLEEP);
  assert(vm != NULL);

  int rc;
  vmem_addr_t addr;
  vmem_size_t size;

  /* add span #1 */
  span_t span1 = {.addr = 2 * quantum, .size = 8 * quantum};
  rc = vmem_add(vm, span1.addr, span1.size, VMEM_NOSLEEP);
  assert(rc == 0);

  /* add span #2 */
  span_t span2 = {.addr = 100 * quantum, .size = 20 * quantum};
  rc = vmem_add(vm, span2.addr, span2.size, VMEM_NOSLEEP);
  assert(rc == 0);

  /* alloc 16 quantums, should return addr from span #2 */
  size = 16 * quantum;
  rc = vmem_alloc(vm, size, VMEM_INSTANTFIT, &addr);
  assert(rc == 0);
  assert_addr_is_in_span(addr, size, &span2);

  /* alloc 8 quantums, should return addr from span #1 */
  size = 8 * quantum;
  rc = vmem_alloc(vm, size, VMEM_INSTANTFIT, &addr);
  assert(rc == 0);
  assert_addr_is_in_span(addr, size, &span1);

  /* alloc 1 quantum, should return addr from span #2 */
  size = 1 * quantum;
  rc = vmem_alloc(vm, size, VMEM_INSTANTFIT, &addr);
  assert(rc == 0);
  assert_addr_is_in_span(addr, size, &span2);

  /* alloc 10 quantums, should fail */
  size = 10 * quantum;
  rc = vmem_alloc(vm, size, VMEM_INSTANTFIT, &addr);
  assert(rc == ENOMEM);

  return KTEST_SUCCESS;
}

KTEST_ADD(vmem, test_vmem, 0);
