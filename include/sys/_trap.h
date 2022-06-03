#ifndef _SYS__TRAP_H_
#define _SYS__TRAP_H_

#include <sys/vm_map.h>
#include <machine/mcontext.h>

/*
 * Interface provided by the generic machine-dependent trap handling module.
 *
 * TODO: we should also have a generic syscall handler.
 */

void page_fault_handler(ctx_t *ctx, u_long exc_code, vaddr_t vaddr,
                        vm_prot_t access);

/*
 * Interface that must be implemented by the architecture-dependnet
 * trap handling module.
 */

const char *exc_str(u_long exc_code);
__noreturn void kernel_oops(ctx_t *ctx);

#endif /* !_SYS__TRAP_H_ */
