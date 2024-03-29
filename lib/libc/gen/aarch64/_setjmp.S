#include <aarch64/asm.h>
#include <sys/syscall.h>

#include "aarch64/assym.h"

ENTRY(_setjmp)
        /* Copy saved registers */
        str     x19, [x0, UC_REGS_X19]
        stp     x20, x21, [x0, UC_REGS_X20]
        stp     x22, x23, [x0, UC_REGS_X22]
        stp     x24, x25, [x0, UC_REGS_X24]
        stp     x26, x27, [x0, UC_REGS_X26]
        stp     x28, x29, [x0, UC_REGS_X28]
        str     lr, [x0, UC_REGS_LR]

        /* sp register is special :( */
        mov     x1, sp
        str     x1, [x0, UC_REGS_SP]

        /* save flags */
        ldr     x1, [x0, UC_FLAGS]
        orr     x1, x1, (_UC_FPU | _UC_CPU)
        str     x1, [x0, UC_FLAGS]

        /* FPU status */
        mrs     x1, fpcr
        str     w1, [x0, UC_FPREGS_FPCR]
        mrs     x1, fpsr
        str     w1, [x0, UC_FPREGS_FPSR]

        stp     q0, q1, [x0, UC_FPREGS_Q0]
        stp     q2, q3, [x0, UC_FPREGS_Q2]
        stp     q4, q5, [x0, UC_FPREGS_Q4]
        stp     q6, q7, [x0, UC_FPREGS_Q6]
        stp     q8, q9, [x0, UC_FPREGS_Q8]
        stp     q10, q11, [x0, UC_FPREGS_Q10]
        stp     q12, q13, [x0, UC_FPREGS_Q12]
        stp     q14, q15, [x0, UC_FPREGS_Q14]
        stp     q16, q17, [x0, UC_FPREGS_Q16]
        stp     q18, q19, [x0, UC_FPREGS_Q18]
        stp     q20, q21, [x0, UC_FPREGS_Q20]
        stp     q22, q23, [x0, UC_FPREGS_Q22]
        stp     q24, q25, [x0, UC_FPREGS_Q24]
        stp     q26, q27, [x0, UC_FPREGS_Q26]
        stp     q28, q29, [x0, UC_FPREGS_Q28]
        stp     q30, q31, [x0, UC_FPREGS_Q30]

        mov     x0, xzr
        ret
END(_setjmp)

ENTRY(_longjmp)
        ldr     x19, [x0, UC_REGS_SP]
        mov     sp, x19
        ldr     x19, [x0, UC_REGS_X19]
        ldp     x20, x21, [x0, UC_REGS_X20]
        ldp     x22, x23, [x0, UC_REGS_X22]
        ldp     x24, x25, [x0, UC_REGS_X24]
        ldp     x26, x27, [x0, UC_REGS_X26]
        ldp     x28, x29, [x0, UC_REGS_X28]
        ldr     lr, [x0, UC_REGS_LR]

        /* FPU status */
        ldr     w2, [x0, UC_FPREGS_FPCR]
        /* msr ignores upper 32 bits for fpcr & fpsr */
        msr     fpcr, x2
        ldr     w2, [x0, UC_FPREGS_FPSR]
        msr     fpsr, x2

        ldp     q0, q1, [x0, UC_FPREGS_Q0]
        ldp     q2, q3, [x0, UC_FPREGS_Q2]
        ldp     q4, q5, [x0, UC_FPREGS_Q4]
        ldp     q6, q7, [x0, UC_FPREGS_Q6]
        ldp     q8, q9, [x0, UC_FPREGS_Q8]
        ldp     q10, q11, [x0, UC_FPREGS_Q10]
        ldp     q12, q13, [x0, UC_FPREGS_Q12]
        ldp     q14, q15, [x0, UC_FPREGS_Q14]
        ldp     q16, q17, [x0, UC_FPREGS_Q16]
        ldp     q18, q19, [x0, UC_FPREGS_Q18]
        ldp     q20, q21, [x0, UC_FPREGS_Q20]
        ldp     q22, q23, [x0, UC_FPREGS_Q22]
        ldp     q24, q25, [x0, UC_FPREGS_Q24]
        ldp     q26, q27, [x0, UC_FPREGS_Q26]
        ldp     q28, q29, [x0, UC_FPREGS_Q28]
        ldp     q30, q31, [x0, UC_FPREGS_Q30]

        mov     x0, x1
        ret
END(_longjmp)
