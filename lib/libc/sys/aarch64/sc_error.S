#include <aarch64/asm.h>
#include <sys/errno.h>

ENTRY(__sc_error)
        sub     sp, sp, #16
        stp     x19, lr, [sp]
        mov     w19, w1
        bl      __errno
        str     w19, [x0]
        mov     x0, #-1
        ldp     x19, lr, [sp]
        add     sp, sp, #16
        ret
END(__sc_error)
