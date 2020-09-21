#include <aarch64/asm.h>
#include <aarch64/armreg.h>
#include <aarch64/pcpu.h>

#include "assym.h"

.macro  SAVE_CTX tmp
        mov     \tmp, sp
        sub     sp, sp, #CTX_SIZE
        stp     lr, \tmp, [sp, #CTX_LR]
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

.macro  LOAD_CTX tmp
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
        ldp     lr, \tmp, [sp, #CTX_LR]
        mov     sp, \tmp
.endm

#
# long ctx_switch(thread_t *from, thread_t *to)
#
ENTRY(ctx_switch)
        # ctx_switch must be called with interrupts disabled
        mrs     x2, daif
        and     x2, x2, #DAIF_I_MASKED
        bne     1f
        hlt     #0

        # don't save context of @from thread if user did not provide one
1:      cbz     x0, .ctx_resume

        # save context of @from thread
.ctx_save:
        SAVE_CTX x2
        mov     x2, sp
        str     x2, [x0, #TD_KCTX]

.ctx_resume:
        # [x8] @to thread
        mov     x8, x1

        # [x9] pointer to @pcpu
        mrs     x9, tpidr_el1

        # update curthread pointer to reference @to thread
        str     x8, [x9, #PCPU_CURTHREAD]

        # switch user space if necessary
        ldr     x0, [x8, #TD_PROC]
        # switching to kernel thread ?
        cbz     x0, 2f
        ldr     x0, [x0, #P_USPACE]
2:      bl      vm_map_activate

        # restore @to thread context
        ldr     x1, [x8, #TD_KCTX]
        mov     sp, x1
        LOAD_CTX x1
        
        ret
END(ctx_switch)
