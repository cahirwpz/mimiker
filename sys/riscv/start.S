#include <riscv/asm.h>

	.org 0x0
	.section .boot

_ENTRY(_start)
	j .
END(_start)

# vim: sw=8 ts=8 et
