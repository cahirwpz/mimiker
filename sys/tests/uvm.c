#include <sys/klog.h>
#include <sys/ktest.h>
#include <sys/uvm_anon.h>

static int test_anon_simple(void) {

  uvm_anon_t *anon = uvm_anon_alloc();
  assert(anon != NULL);

  uvm_anon_lock(anon);

  uvm_anon_hold(anon);
  uvm_anon_hold(anon);
  assert(anon->an_ref == 2);

  uvm_anon_drop(anon);
  assert(anon->an_ref == 1);

  uvm_anon_lock(anon);
  uvm_anon_drop(anon);

  return KTEST_SUCCESS;
}

KTEST_ADD(uvm_anon_simple, test_anon_simple, 0);
