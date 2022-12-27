#include <mips/asm.h>
#include <mips/regdef.h>
#include <sys/errno.h>

        .set noreorder
        .globl _C_LABEL(__sc_error)

LEAF(__sc_missing)
        li      v0, -1
        b       _C_LABEL(__sc_error)
        li      v1, ENOSYS
END(__sc_missing)
