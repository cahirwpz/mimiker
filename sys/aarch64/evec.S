#include <aarch64/asm.h>

#include "assym.h"

/* Heavily inspired by FreeBSD sys/arm64/arm64/exceptions.S */

.macro  save_kern_ctx
        sub     sp, sp, #(EXC_SIZE)
        stp     x28, x29, [sp, #(EXC_X + 28 * 8)]
        stp     x26, x27, [sp, #(EXC_X + 26 * 8)]
        stp     x24, x25, [sp, #(EXC_X + 24 * 8)]
        stp     x22, x23, [sp, #(EXC_X + 22 * 8)]
        stp     x20, x21, [sp, #(EXC_X + 20 * 8)]
        stp     x18, x19, [sp, #(EXC_X + 18 * 8)]
        stp     x16, x17, [sp, #(EXC_X + 16 * 8)]
        stp     x14, x15, [sp, #(EXC_X + 14 * 8)]
        stp     x12, x13, [sp, #(EXC_X + 12 * 8)]
        stp     x10, x11, [sp, #(EXC_X + 10 * 8)]
        stp     x8,  x9,  [sp, #(EXC_X + 8  * 8)]
        stp     x6,  x7,  [sp, #(EXC_X + 6  * 8)]
        stp     x4,  x5,  [sp, #(EXC_X + 4  * 8)]
        stp     x2,  x3,  [sp, #(EXC_X + 2  * 8)]
        stp     x0,  x1,  [sp, #(EXC_X + 0  * 8)]
        mrs     x10, far_el1
        mrs     x11, elr_el1
        stp     x10, x11, [sp, #(EXC_FAR)]
        mrs     x12, spsr_el1
        mrs     x13, esr_el1
        stp     w12, w13, [sp, #(EXC_SPSR)]
        add     x18, sp, #(EXC_SIZE)
        stp     x18, lr, [sp, #(EXC_SP)]
.endm

.macro  load_kern_ctx
        ldp     x18,  lr, [sp, #(EXC_SP)]
        ldr     x10, [sp, #(EXC_ELR)]
        msr     elr_el1, x10
        ldr     w11, [sp, #(EXC_SPSR)]
        msr     spsr_el1, x11
        ldp     x0,  x1,  [sp, #(EXC_X + 0  * 8)]
        ldp     x2,  x3,  [sp, #(EXC_X + 2  * 8)]
        ldp     x4,  x5,  [sp, #(EXC_X + 4  * 8)]
        ldp     x6,  x7,  [sp, #(EXC_X + 6  * 8)]
        ldp     x8,  x9,  [sp, #(EXC_X + 8  * 8)]
        ldp     x10, x11, [sp, #(EXC_X + 10 * 8)]
        ldp     x12, x13, [sp, #(EXC_X + 12 * 8)]
        ldp     x14, x15, [sp, #(EXC_X + 14 * 8)]
        ldp     x16, x17, [sp, #(EXC_X + 16 * 8)]
        mov     sp, x18
.endm

ASENTRY(handle_kern_trap)
        save_kern_ctx
        /* bl aarch64_trap_handler */
        load_kern_ctx
        eret
ASEND(handle_kern_trap)

ASENTRY(handle_kern_irq)
        save_kern_ctx
        /* bl aarch64_irq_handler */
        load_kern_ctx
        eret
ASEND(handle_kern_irq)

.macro  VECKERN name
        B       handle_kern_\name
        .align  7
.endm

.macro  VECUSER name
        B       .
        .align  7
.endm

.macro  VEMPTY
        B       .
        .align  7
.endm

        .section .text
        .global exception_vectors

        .align 11
exception_vectors:
        VEMPTY  /* Synchronous EL1t */
        VEMPTY  /* IRQ EL1t */
        VEMPTY  /* FIQ EL1t */
        VEMPTY  /* Error EL1t */

        VECKERN trap  /* Synchronous EL1h */
        VECKERN irq  /* IRQ EL1h */
        VEMPTY  /* FIQ EL1h */
        VEMPTY  /* Error EL1h */

        VECUSER trap  /* Synchronous 64-bit EL0 */
        VECUSER irq  /* IRQ 64-bit EL0 */
        VEMPTY  /* FIQ 64-bit EL0 */
        VEMPTY  /* Error 64-bit EL0 */

        VEMPTY  /* Synchronous 32-bit EL0 */
        VEMPTY  /* IRQ 32-bit EL0 */
        VEMPTY  /* FIQ 32-bit EL0 */
        VEMPTY  /* Error 32-bit EL0 */

        .section .boot.text
        .global hypervisor_vectors

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

# vim: sw=8 ts=8 et
