#include <aarch64/asm.h>
#include <sys/syscall.h>

        .global sigcode
        .global esigcode

ENTRY(sigcode)
        svc #SYS_sigreturn
        b 0
esigcode:
END(sigcode)

# vim: sw=8 ts=8 et
