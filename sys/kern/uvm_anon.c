#define KL_LOG KL_VM
#include <sys/mimiker.h>
#include <sys/klog.h>
#include <sys/pool.h>
#include <sys/vm.h>
#include <sys/vm_physmem.h>
#include <sys/uvm_anon.h>

static POOL_DEFINE(P_ANON, "uvm_anon", sizeof(uvm_anon_t));

uvm_anon_t *uvm_anon_alloc(void) {
  uvm_anon_t *anon = pool_alloc(P_ANON, M_ZERO);
  mtx_init(&anon->an_lock, 0);
  return anon;
}

void uvm_anon_lock(uvm_anon_t *anon) {
  mtx_lock(&anon->an_lock);
}

void uvm_anon_unlock(uvm_anon_t *anon) {
  mtx_unlock(&anon->an_lock);
}

void uvm_anon_hold(uvm_anon_t *anon) {
  assert(mtx_owned(&anon->an_lock));
  anon->an_ref++;
}

static void anon_free(uvm_anon_t *anon) {
  /* TODO(fz): free anon->an_page */
  pool_free(P_ANON, anon);
}

void uvm_anon_drop(uvm_anon_t *anon) {
  assert(mtx_owned(&anon->an_lock));
  anon->an_ref--;

  uvm_anon_unlock(anon);
  if (anon->an_ref == 0)
    anon_free(anon);
}
