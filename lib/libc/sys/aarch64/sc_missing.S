#include <aarch64/asm.h>
#include <sys/errno.h>

ENTRY(__sc_missing)
        mov     x0, #-1
        mov     x1, #ENOSYS
        ret
END(__sc_missing)
