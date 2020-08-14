#include <aarch64/asm.h>

#include "assym.h"

/* Copy from FreeBSD. */

.macro  save_registers el
.if \el == 1
    mov x18, sp
    sub sp, sp, #128 /* Czemu 128? */
.endif
    sub sp, sp, #(EXC_SIZE + 16) /* Czemu +16? */
    stp x29, x30, [sp, #(EXC_SIZE)]
    stp x28, x29, [sp, #(EXC_X + 28 * 8)]
    stp x26, x27, [sp, #(EXC_X + 26 * 8)]
    stp x24, x25, [sp, #(EXC_X + 24 * 8)]
    stp x22, x23, [sp, #(EXC_X + 22 * 8)]
    stp x20, x21, [sp, #(EXC_X + 20 * 8)]
    stp x18, x19, [sp, #(EXC_X + 18 * 8)]
    stp x16, x17, [sp, #(EXC_X + 16 * 8)]
    stp x14, x15, [sp, #(EXC_X + 14 * 8)]
    stp x12, x13, [sp, #(EXC_X + 12 * 8)]
    stp x10, x11, [sp, #(EXC_X + 10 * 8)]
    stp x8,  x9,  [sp, #(EXC_X + 8  * 8)]
    stp x6,  x7,  [sp, #(EXC_X + 6  * 8)]
    stp x4,  x5,  [sp, #(EXC_X + 4  * 8)]
    stp x2,  x3,  [sp, #(EXC_X + 2  * 8)]
    stp x0,  x1,  [sp, #(EXC_X + 0  * 8)]
    mrs x10, elr_el1
    mrs x11, spsr_el1
    mrs x12, esr_el1
.if \el == 0
    mrs x18, sp_el0
.endif
    str x10, [sp, #(EXC_ELR)]
    stp w11, w12, [sp, #(EXC_SPSR)]
    stp x18,  lr, [sp, #(EXC_SP)]
    mrs x18, tpidr_el1
    add x29, sp, #(EXC_SIZE)
.if \el == 0
    /* Apply the SSBD (CVE-2018-3639) workaround if needed */
    ldr x1, [x18, #PC_SSBD]
    cbz x1, 1f
    mov w0, #1
    blr x1
1:

    ldr x0, [x18, #(PC_CURTHREAD)]
    bl  dbg_monitor_enter
.endif
    msr daifclr, #8 /* Enable the debug exception */
.endm

.macro  restore_registers el
.if \el == 1
    /*
     * Disable interrupts and debug exceptions, x18 may change in the
     * interrupt exception handler.  For EL0 exceptions, do_ast already
     * did this.
     */
    msr daifset, #10
.endif
.if \el == 0
    ldr x0, [x18, #PC_CURTHREAD]
    mov x1, sp
    bl  dbg_monitor_exit

    /* Remove the SSBD (CVE-2018-3639) workaround if needed */
    ldr x1, [x18, #PC_SSBD]
    cbz x1, 1f
    mov w0, #0
    blr x1
1:
.endif
    ldp x18,  lr, [sp, #(EXC_SP)]
    ldp x10, x11, [sp, #(EXC_ELR)]
.if \el == 0
    msr sp_el0, x18
.endif
    msr spsr_el1, x11
    msr elr_el1, x10
    ldp x0,  x1,  [sp, #(EXC_X + 0  * 8)]
    ldp x2,  x3,  [sp, #(EXC_X + 2  * 8)]
    ldp x4,  x5,  [sp, #(EXC_X + 4  * 8)]
    ldp x6,  x7,  [sp, #(EXC_X + 6  * 8)]
    ldp x8,  x9,  [sp, #(EXC_X + 8  * 8)]
    ldp x10, x11, [sp, #(EXC_X + 10 * 8)]
    ldp x12, x13, [sp, #(EXC_X + 12 * 8)]
    ldp x14, x15, [sp, #(EXC_X + 14 * 8)]
    ldp x16, x17, [sp, #(EXC_X + 16 * 8)]
.if \el == 0
    /*
     * We only restore the callee saved registers when returning to
     * userland as they may have been updated by a system call or signal.
     */
    ldp x18, x19, [sp, #(EXC_X + 18 * 8)]
    ldp x20, x21, [sp, #(EXC_X + 20 * 8)]
    ldp x22, x23, [sp, #(EXC_X + 22 * 8)]
    ldp x24, x25, [sp, #(EXC_X + 24 * 8)]
    ldp x26, x27, [sp, #(EXC_X + 26 * 8)]
    ldp x28, x29, [sp, #(EXC_X + 28 * 8)]
.else
    ldr      x29, [sp, #(EXC_X + 29 * 8)]
.endif
.if \el == 0
    add sp, sp, #(EXC_SIZE + 16)
.else
    mov sp, x18
    mrs x18, tpidr_el1
.endif
.endm

.macro  VECTOR  name
        .align 7
        b   handle_\name
.endm

#define VEMPTY                                                             \
        B       .;                                                             \
        .align 7

        .section .text
        .global exception_vectors

        .align 11
exception_vectors:
        VEMPTY  /* Synchronous EL1t */
        VEMPTY  /* IRQ EL1t */
        VEMPTY  /* FIQ EL1t */
        VEMPTY  /* Error EL1t */

        VECTOR  kern_trap  /* Synchronous EL1h */
        VECTOR  kern_irq  /* IRQ EL1h */
        VEMPTY  /* FIQ EL1h */
        VEMPTY  /* Error EL1h */

        VECTOR  user_trap  /* Synchronous 64-bit EL0 */
        VECTOR  user_irq  /* IRQ 64-bit EL0 */
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
