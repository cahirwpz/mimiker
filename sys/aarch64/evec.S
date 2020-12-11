#include <aarch64/asm.h>

#include "assym.h"

/* Heavily inspired by FreeBSD sys/arm64/arm64/exceptions.S */

.macro  save_ctx el
        sub     sp, sp, #CTX_SIZE
        .cfi_def_cfa_offset 0
        stp     x28, x29, [sp, #CTX_X28]
        .cfi_rel_offset x28, CTX_X28
        .cfi_rel_offset x29, CTX_X29
        stp     x26, x27, [sp, #CTX_X26]
        .cfi_rel_offset x26, CTX_X26
        .cfi_rel_offset x27, CTX_X27
        stp     x24, x25, [sp, #CTX_X24]
        .cfi_rel_offset x24, CTX_X24
        .cfi_rel_offset x25, CTX_X25
        stp     x22, x23, [sp, #CTX_X22]
        .cfi_rel_offset x22, CTX_X22
        .cfi_rel_offset x23, CTX_X23
        stp     x20, x21, [sp, #CTX_X20]
        .cfi_rel_offset x20, CTX_X20
        .cfi_rel_offset x21, CTX_X21
        stp     x18, x19, [sp, #CTX_X18]
        .cfi_rel_offset x18, CTX_X18
        .cfi_rel_offset x19, CTX_X19
        stp     x16, x17, [sp, #CTX_X16]
        .cfi_rel_offset x16, CTX_X16
        .cfi_rel_offset x17, CTX_X17
        stp     x14, x15, [sp, #CTX_X14]
        .cfi_rel_offset x14, CTX_X14
        .cfi_rel_offset x15, CTX_X15
        stp     x12, x13, [sp, #CTX_X12]
        .cfi_rel_offset x12, CTX_X12
        .cfi_rel_offset x13, CTX_X13
        stp     x10, x11, [sp, #CTX_X10]
        .cfi_rel_offset x10, CTX_X10
        .cfi_rel_offset x11, CTX_X11
        stp     x8,  x9,  [sp, #CTX_X8]
        .cfi_rel_offset x8, CTX_X8
        .cfi_rel_offset x9, CTX_X9
        stp     x6,  x7,  [sp, #CTX_X6]
        .cfi_rel_offset x6, CTX_X6
        .cfi_rel_offset x7, CTX_X7
        stp     x4,  x5,  [sp, #CTX_X4]
        .cfi_rel_offset x4, CTX_X4
        .cfi_rel_offset x5, CTX_X5
        stp     x2,  x3,  [sp, #CTX_X2]
        .cfi_rel_offset x2, CTX_X2
        .cfi_rel_offset x3, CTX_X3
        stp     x0,  x1,  [sp, #CTX_X0]
        .cfi_rel_offset x0, CTX_X0
        .cfi_rel_offset x1, CTX_X1
        mrs     x10, elr_el1
        mrs     x11, spsr_el1
        stp     x10, x11, [sp, #CTX_ELR]
.if \el == 1
        add     x18, sp, #CTX_SIZE
.else
        mrs     x18, sp_el0
.endif
        stp      lr, x18, [sp, #CTX_LR]
        .cfi_rel_offset lr, CTX_LR
        .cfi_rel_offset sp, CTX_SP
        mrs     x18, tpidr_el1
.endm

.macro  load_ctx el
        msr     daifset, #2
.if \el == 0
        mrs     x18, tpidr_el1
        ldr     x18, [x18, #PCPU_CURTHREAD]
        ldr     x18, [x18, #TD_UCTX]
        mov     sp,  x18
.endif
        ldp     lr, x18, [sp, #CTX_LR]
.if \el == 0
        msr     sp_el0, x18
.endif
        ldr     x10, [sp, #CTX_ELR]
        msr     elr_el1, x10
        ldr     w11, [sp, #CTX_SPSR]
        msr     spsr_el1, x11
        ldp     x0,  x1,  [sp, #CTX_X0]
        ldp     x2,  x3,  [sp, #CTX_X2]
        ldp     x4,  x5,  [sp, #CTX_X4]
        ldp     x6,  x7,  [sp, #CTX_X6]
        ldp     x8,  x9,  [sp, #CTX_X8]
        ldp     x10, x11, [sp, #CTX_X10]
        ldp     x12, x13, [sp, #CTX_X12]
        ldp     x14, x15, [sp, #CTX_X14]
        ldp     x16, x17, [sp, #CTX_X16]
.if \el == 0
        ldp     x18, x19, [sp, #CTX_X18]
        ldp     x20, x21, [sp, #CTX_X20]
        ldp     x22, x23, [sp, #CTX_X22]
        ldp     x24, x25, [sp, #CTX_X24]
        ldp     x26, x27, [sp, #CTX_X26]
        ldp     x28, x29, [sp, #CTX_X28]
        add     sp, sp, #CTX_SIZE
.else
        ldr     x29, [sp, #CTX_FP]
        mov     sp, x18
        mrs     x18, tpidr_el1
.endif
.endm

        .global kern_exc_leave
        .global user_exc_leave

ENTRY(handle_kern_trap)
        .cfi_signal_frame
        save_ctx 1
        mov     x0, sp
        bl      kern_trap_handler
kern_exc_leave:
        load_ctx 1
        ERET
END(handle_kern_trap)

ENTRY(handle_kern_intr)
        .cfi_signal_frame
        save_ctx 1
        mov     x0, sp
        bl      intr_root_handler
        load_ctx 1
        ERET
END(handle_kern_intr)

ENTRY(handle_user_trap)
        .cfi_signal_frame
        save_ctx 0
        mov     x0, sp
        bl      user_trap_handler
user_exc_leave:
        load_ctx 0
        ERET
END(handle_user_trap)

ENTRY(handle_user_intr)
        .cfi_signal_frame
        save_ctx 0
        mov     x0, sp
        bl      intr_root_handler
        load_ctx 0
        ERET
END(handle_user_intr)

.macro  VECKERN name
        b       handle_kern_\name
        .align  7
.endm

.macro  VECUSER name
        b       handle_user_\name
        .align  7
.endm

.macro  VEMPTY
        hlt     #0
        .align  7
.endm

        .section .text
        .global exception_vectors
        .type exception_vectors,@object
        .align 11
exception_vectors:
        VEMPTY  /* Synchronous EL1t */
        VEMPTY  /* IRQ EL1t */
        VEMPTY  /* FIQ EL1t */
        VEMPTY  /* Error EL1t */

        VECKERN trap  /* Synchronous EL1h */
        VECKERN intr  /* IRQ EL1h */
        VEMPTY  /* FIQ EL1h */
        VEMPTY  /* Error EL1h */

        VECUSER trap  /* Synchronous 64-bit EL0 */
        VECUSER intr  /* IRQ 64-bit EL0 */
        VEMPTY  /* FIQ 64-bit EL0 */
        VEMPTY  /* Error 64-bit EL0 */

        VEMPTY  /* Synchronous 32-bit EL0 */
        VEMPTY  /* IRQ 32-bit EL0 */
        VEMPTY  /* FIQ 32-bit EL0 */
        VEMPTY  /* Error 32-bit EL0 */
_END(exception_vectors)

        .section .boot.text
        .global hypervisor_vectors
        .type hypervisor_vectors,@object
        .align 11
hypervisor_vectors:
        VEMPTY  /* Synchronous EL2t */
        VEMPTY  /* IRQ EL2t */
        VEMPTY  /* FIQ EL2t */
        VEMPTY  /* Error EL2t */

        VEMPTY  /* Synchronous EL2h */
        VEMPTY  /* IRQ EL2h */
        VEMPTY  /* FIQ EL2h */
        VEMPTY  /* Error EL2h */

        VEMPTY  /* Synchronous 64-bit EL1 */
        VEMPTY  /* IRQ 64-bit EL1 */
        VEMPTY  /* FIQ 64-bit EL1 */
        VEMPTY  /* Error 64-bit EL1 */

        VEMPTY  /* Synchronous 32-bit EL1 */
        VEMPTY  /* IRQ 32-bit EL1 */
        VEMPTY  /* FIQ 32-bit EL1 */
        VEMPTY  /* Error 32-bit EL1 */
_END(hypervisor_vectors)

# vim: sw=8 ts=8 et
