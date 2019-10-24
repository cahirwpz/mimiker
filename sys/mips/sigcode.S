#include <mips/asm.h>
#include <mips/regdef.h>
#include <sys/syscall.h>

        .set noreorder

LEAF_NOPROFILE(sigcode)
        li v0, SYS_sigreturn
        syscall
        # just in case sigreturn fails
        j 0     # TODO: replace with "break 0" when SIGTRAP is implemented
XLEAF(esigcode)
END(sigcode)

# vim: sw=8 ts=8 et
