#include <aarch64/asm.h>
#include <sys/syscall.h>

        .global sigcode
        .global esigcode

ENTRY(sigcode)
        mov     x0, sp       /* address of ucontext_t to restore */
        svc     #SYS_sigreturn
        brk     #0
EXPORT(esigcode)
END(sigcode)
