#include <aarch64/asm.h>

        .section .boot, "ax"

#define STACK_START_SIZE 0x38

ASENTRY(_start)
    /* Save atags. */
    MOV  x27, x0

    /* Get CPU number. */
    MRS  x3, MPIDR_EL1
    AND  x3, x3, #3
    
    ADD  x3, x3, #0x1
    MOV  x4, STACK_START_SIZE

    ADD  x4, x4, 0x18
    MOV  SP, x4

    BL   aarch64_init
ASEND(_start)
