#include <aarch64/asm.h>

#define IMAGE_OFFSET 0
#define IMAGE_SIZE 0
#define IMAGE_FLAGS 0

        .section .boot, "ax"
        .extern _boot_stack

ASENTRY(_start)
        /* Based on  locore.S from FreeBSD. */
        b       1f
        .long   0
        .quad   IMAGE_OFFSET
        .quad   IMAGE_SIZE
        .quad   IMAGE_FLAGS
        .quad   0
        .quad   0
        .quad   0
        .long   0x644d5241
        .long   0
1:
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
