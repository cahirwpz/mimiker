#include <aarch64/asm.h>
#include <sys/ucontext.h>
#include <sys/syscall.h>

#include "aarch64/assym.h"

ENTRY(_setjmp)
        /* Copy saved registers */
        str     x19, [x0, _OFFSETOF_SC_REGS_X19]
        str     x20, [x0, _OFFSETOF_SC_REGS_X20]
        str     x21, [x0, _OFFSETOF_SC_REGS_X21]
        str     x22, [x0, _OFFSETOF_SC_REGS_X22]
        str     x23, [x0, _OFFSETOF_SC_REGS_X23]
        str     x24, [x0, _OFFSETOF_SC_REGS_X24]
        str     x25, [x0, _OFFSETOF_SC_REGS_X25]
        str     x26, [x0, _OFFSETOF_SC_REGS_X26]
        str     x27, [x0, _OFFSETOF_SC_REGS_X27]
        str     x28, [x0, _OFFSETOF_SC_REGS_X28]
        str     x29, [x0, _OFFSETOF_SC_REGS_X29]
        str     lr, [x0, _OFFSETOF_SC_REGS_LR]

        mov     x1, sp
        str     x1, [x0, _OFFSETOF_SC_REGS_SP]
        
        ldr     x1, [x0, _OFFSETOF_SC_REGS_X29]

        ldr     x1, [x0, _OFFSETOF_SC_FLAGS]
        orr     x1, x1, (_UC_FPU | _UC_CPU)
        str     x1, [x0, _OFFSETOF_SC_FLAGS]

        /* TODO(pj): FPU */
        mov     x0, xzr
        ret
END(_setjmp)

ENTRY(_longjmp)
        ldr     x19, [x0, _OFFSETOF_SC_REGS_SP]
        mov     sp, x19
        ldr     x19, [x0, _OFFSETOF_SC_REGS_X19]
        ldr     x20, [x0, _OFFSETOF_SC_REGS_X20]
        ldr     x21, [x0, _OFFSETOF_SC_REGS_X21]
        ldr     x22, [x0, _OFFSETOF_SC_REGS_X22]
        ldr     x23, [x0, _OFFSETOF_SC_REGS_X23]
        ldr     x24, [x0, _OFFSETOF_SC_REGS_X24]
        ldr     x25, [x0, _OFFSETOF_SC_REGS_X25]
        ldr     x26, [x0, _OFFSETOF_SC_REGS_X26]
        ldr     x27, [x0, _OFFSETOF_SC_REGS_X27]
        ldr     x28, [x0, _OFFSETOF_SC_REGS_X28]
        ldr     x29, [x0, _OFFSETOF_SC_REGS_X29]
        ldr     lr, [x0, _OFFSETOF_SC_REGS_LR]

        /* TODO(pj): FPU */
        mov     x0, x1
        ret
END(_longjmp)
