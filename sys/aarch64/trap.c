#include <sys/klog.h>
#include <sys/context.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <sys/pmap.h>
#include <sys/vm_physmem.h>
#include <sys/vm_map.h>
#include <aarch64/armreg.h>
#include <aarch64/context.h>
#include <aarch64/pmap.h>

static __noreturn void kernel_oops(ctx_t *ctx) {
  kprintf("KERNEL PANIC!!! \n");
  panic();
}

static void tlb_exception_handler(ctx_t *ctx, uint32_t exception, vaddr_t
                                  vaddr) {
  thread_t *td = thread_self();

  klog("%x at $%lx, caused by reference to $%lx!", exception, _REG(ctx, PC),
       vaddr);

  pmap_t *pmap = pmap_lookup(vaddr);
  if (!pmap) {
    klog("No physical map defined for %lx address!", vaddr);
    goto fault;
  }

  paddr_t pa;
  if (pmap_extract(pmap, vaddr, &pa)) {
    /* TODO(pj) implement it */
    kernel_oops(ctx);
  }

  vm_map_t *vmap = vm_map_lookup(vaddr);
  if (!vmap) {
    klog("No virtual address space defined for %lx!", vaddr);
    goto fault;
  }

fault:
  if (td->td_onfault) {
    _REG(ctx, PC) = td->td_onfault;
    td->td_onfault = 0;
  } else {
    /* TODO(pj) send a segmentation fault signal to the user program. */
    kernel_oops(ctx);
  }
}

void cpu_user_trap_handler(user_ctx_t *uctx) {
  panic("cpu_user_trap_handler");
}

/* el1h_sync */
void cpu_kern_trap_handler(ctx_t *ctx) {
  register_t esr = READ_SPECIALREG(esr_el1);
  uint32_t exception = ESR_ELx_EXCEPTION(esr);

  switch (exception) {
  case EXCP_INSN_ABORT:
  case EXCP_DATA_ABORT:
    tlb_exception_handler(ctx, exception, READ_SPECIALREG(far_el1));
    break;
  default:
    kernel_oops(ctx);
  }
}
