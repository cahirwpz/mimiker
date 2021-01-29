#include <aarch64/asm.h>
#include <aarch64/armreg.h>
#include <aarch64/pcpu.h>

#include "assym.h"

.macro  SAVE_CTX
        str     lr, [sp, #CTX_PC]
        stp     x28, x29, [sp, #CTX_X28]
        stp     x26, x27, [sp, #CTX_X26]
        stp     x24, x25, [sp, #CTX_X24]
        stp     x22, x23, [sp, #CTX_X22]
        stp     x20, x21, [sp, #CTX_X20]
        stp     x18, x19, [sp, #CTX_X18]
        stp     x16, x17, [sp, #CTX_X16]
        stp     x14, x15, [sp, #CTX_X14]
        stp     x12, x13, [sp, #CTX_X12]
        stp     x10, x11, [sp, #CTX_X10]
        stp      x8,  x9, [sp, #CTX_X8]
.endm

/* 
 * WARNING!
 * tmp is a register number.
 */
.macro  save_fpu_ctx src tmp
        stp     q0,  q1,  [\src, FPU_CTX_Q0]
        stp     q2,  q3,  [\src, FPU_CTX_Q2]
        stp     q4,  q5,  [\src, FPU_CTX_Q4]
        stp     q6,  q7,  [\src, FPU_CTX_Q6]
        stp     q8,  q9,  [\src, FPU_CTX_Q8]
        stp     q10, q11, [\src, FPU_CTX_Q10]
        stp     q12, q13, [\src, FPU_CTX_Q12]
        stp     q14, q15, [\src, FPU_CTX_Q14]
        stp     q16, q17, [\src, FPU_CTX_Q16]
        stp     q18, q19, [\src, FPU_CTX_Q18]
        stp     q20, q21, [\src, FPU_CTX_Q20]
        stp     q22, q23, [\src, FPU_CTX_Q22]
        stp     q24, q25, [\src, FPU_CTX_Q24]
        stp     q26, q27, [\src, FPU_CTX_Q26]
        stp     q28, q29, [\src, FPU_CTX_Q28]
        stp     q30, q31, [\src, FPU_CTX_Q30]
        mrs     x\tmp, fpcr
        str     w\tmp, [\src, FPU_CTX_FPCR]
        mrs     x\tmp, fpsr
        str     w\tmp, [\src, FPU_CTX_FPSR]
.endm

.macro  LOAD_CTX
        ldp      x8,  x9, [sp, #CTX_X8]
        ldp     x10, x11, [sp, #CTX_X10]
        ldp     x12, x13, [sp, #CTX_X12]
        ldp     x14, x15, [sp, #CTX_X14]
        ldp     x16, x17, [sp, #CTX_X16]
        ldp     x18, x19, [sp, #CTX_X18]
        ldp     x20, x21, [sp, #CTX_X20]
        ldp     x22, x23, [sp, #CTX_X22]
        ldp     x24, x25, [sp, #CTX_X24]
        ldp     x26, x27, [sp, #CTX_X26]
        ldp     x28, x29, [sp, #CTX_X28]
        ldr     lr, [sp, #CTX_PC]
.endm

#
# long ctx_switch(thread_t *from, thread_t *to)
#
ENTRY(ctx_switch)
        # ctx_switch must be called with interrupts disabled
        mrs     x2, daif
        and     x2, x2, #DAIF_I_MASKED
        cmp     x2, xzr
        bne     .ctx_save
        hlt     #0

        # save context of @from thread
.ctx_save:
        ldr     w3, [x0, #TD_PFLAGS]
        and     w2, w3, #TDP_FPUINUSE|TDP_FPUCTXSAVED
        mov     w4, #TDP_FPUINUSE
        cmp     w2, w4
        bne     .skip_fpu_save

        orr     w3, w3, #TDP_FPUCTXSAVED
        str     w3, [x0, #TD_PFLAGS]

        /* enable FPU and save context */
        /* thread_t::tdp_flags is a volatile unsigned - use 32-bit */
        mrs     x2, cpacr_el1
        and     x2, x2, ~CPACR_FPEN_MASK
        orr     x2, x2, CPACR_FPEN_TRAP_NONE
        msr     cpacr_el1, x2

        ldr     x2, [x0, #TD_UCTX]
        save_fpu_ctx x2, 3

        /* disable FPU */
        mrs     x2, cpacr_el1
        and     x2, x2, ~CPACR_FPEN_MASK
        msr     cpacr_el1, x2


.skip_fpu_save:
        sub     sp, sp, #CTX_SIZE
        SAVE_CTX
        mov     x2, sp
        str     x2, [x0, #TD_KCTX]

.ctx_resume:
        # switch stack pointer to @to thread
        ldr     x2, [x1, #TD_KCTX]
        mov     sp, x2

        # update curthread pointer to reference @to thread
        mrs     x2, tpidr_el1
        str     x1, [x2, #PCPU_CURTHREAD]

        # switch user space if necessary
        mov     x0, x1
        bl      vm_map_switch

        # restore @to thread context
        LOAD_CTX
        add     sp, sp, #CTX_SIZE
        
        ret
END(ctx_switch)

# vim: sw=8 ts=8 et
