#include <aarch64/asm.h>

        .section .boot, "ax"

/* we assume that memory between 0x0 and 0x100 is unused. */
#define STACK_START_SIZE 0x40

ASENTRY(_start)
        /* Save atags. */
        MOV     x27, x0

        /* Get CPU number. */
        MRS     x3, MPIDR_EL1
        AND     x3, x3, #3
        CMP     x3, #0
        BNE     .

        /* Setup initial stack. */
        ADD     x3, x3, #0x1
        MOV     x4, STACK_START_SIZE
        MUL     x4, x3, x4
        MOV     sp, x4

        BL      aarch64_init

        B       .
ASEND(_start)

# vim: sw=8 ts=8 et
