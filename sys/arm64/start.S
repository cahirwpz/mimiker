#include <arm64/asm.h>

        .section .boot, "ax"

ENTRY(_start)
loop:
    nop
    b loop
END(_start)
