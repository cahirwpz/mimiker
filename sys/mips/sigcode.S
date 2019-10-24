#include <mips/asm.h>
#include <mips/regdef.h>
#include <sys/syscall.h>

        .set noreorder

LEAF_NOPROFILE(sigcode)
        li v0, SYS_sigreturn
        syscall
        j 0                     # just in case sigreturn fails
XLEAF(esigcode)
END(sigcode)

# vim: sw=8 ts=8 et
