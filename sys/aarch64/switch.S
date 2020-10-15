#include <aarch64/asm.h>
#include <aarch64/armreg.h>
#include <aarch64/pcpu.h>

#include "assym.h"

.macro  SAVE_CTX
        str     lr, [sp, #CTX_LR]
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
        ldr     lr, [sp, #CTX_LR]
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
