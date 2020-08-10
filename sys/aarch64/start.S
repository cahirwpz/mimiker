#include <aarch64/asm.h>

        .section .boot, "ax"
        .extern _boot_stack

ASENTRY(_start)
        /* Save atags. */
        MOV     x27, x0

        /* Get CPU number. */
        MRS     x3, MPIDR_EL1
        AND     x3, x3, #3
        CMP     x3, #0
        BNE     .

        /* Setup initial stack. */
        ADRP    x3, _boot_stack
        MOV     sp, x3

        BL      aarch64_init

        B       .
ASEND(_start)

# vim: sw=8 ts=8 et
