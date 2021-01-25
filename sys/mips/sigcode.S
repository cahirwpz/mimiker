#include <mips/asm.h>
#include <mips/regdef.h>
#include <sys/syscall.h>

#include "assym.h"

        .set noreorder

LEAF_NOPROFILE(sigcode)
        addi    a0, sp, 4               # address of ucontext_t to restore
        li      v0, SYS_sigreturn
        syscall
        # just in case sigreturn fails
        break   0

# esigcode is used just to compute size of this code.
EXPORT(esigcode)
END(sigcode)
