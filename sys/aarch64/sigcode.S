#include <aarch64/asm.h>
#include <sys/syscall.h>

        .global sigcode
        .global esigcode

ENTRY(sigcode)
        add x0, sp, 8
        svc #SYS_sigreturn
        b 0
esigcode:
END(sigcode)

# vim: sw=8 ts=8 et
