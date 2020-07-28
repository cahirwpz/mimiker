#include <aarch64/asm.h>

        .section .boot, "ax"

ASENTRY(_start)
loop:
    nop
    b loop
ASEND(_start)
