#define KL_LOG KL_VM
#include <sys/context.h>
#include <sys/errno.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/pmap.h>
#include <sys/thread.h>
#include <sys/_trap.h>

void page_fault_handler(ctx_t *ctx, u_long exc_code, vaddr_t vaddr,
                        vm_prot_t access) {
  thread_t *td = thread_self();

  char buf[32];
  const char *estr = exc_str(exc_code);
  if (!estr) {
    snprintf(buf, sizeof(buf), "%ld", exc_code);
    estr = buf;
  }

  void *epc = (void *)_REG(ctx, PC);

  klog("%s at %p, caused by reference to %lx!", exc_str, epc, vaddr);

  pmap_t *pmap = pmap_lookup(vaddr);
  if (!pmap) {
    klog("No physical map defined for %lx address!", vaddr);
    goto fault;
  }

  int error = pmap_emulate_bits(pmap, vaddr, access);
  if (!error)
    return;

  if (error == EACCES || error == EINVAL)
    goto fault;

  vm_map_t *vmap = vm_map_user();
  assert(vmap);

  if (!vm_page_fault(vmap, vaddr, access))
    return;

fault:
  if (td->td_onfault) {
    /* Handle copyin/copyout faults. */
    _REG(ctx, PC) = td->td_onfault;
  } else if (user_mode_p(ctx)) {
    /* Send a segmentation fault signal to the user program. */
    sig_trap(ctx, SIGSEGV);
  } else {
    /* Panic when kernel-mode thread uses wrong pointer. */
    kernel_oops(ctx);
  }
}
