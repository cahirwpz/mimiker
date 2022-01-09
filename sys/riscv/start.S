#include <riscv/abi.h>
#include <riscv/asm.h>

#include "assym.h"

#define INITSTACK_SIZE 512

	.org 0x0
	.section .boot
/*
 * The following entry point is used when booting via SBI firmware.
 * It must be placed at the beginning of the .boot section.
 * Hart start register state:
 *  - satp = 0 (i.e. MMU's disabled)
 *  - sstatus.SIE = 0 (i.e. supervisor interrupts are disabled)
 *  - a0 = hart ID
 *  - a1 = dtbp
 *  - all other registers remain in an undefined state
 */
_ENTRY(_start)
	LOAD_GP()

	/* 
	 * NOTE: In an actual SMP system, we could have multiple harts
	 * entering the kernel simultaneously. In such case,
	 * we would have to choose the hart to perform the global startup.
	 * FTTB, we assume a single hart of ID 0.
	 */
	bnez a0, halt

	/* Move to the initial stack. */
	PTR_LA sp, initstack_end

	mv a0, a1
	tail riscv_init
halt:
	wfi
	j halt
_END(_start)

	.section .boot.data

	.align STACK_ALIGN
initstack:
	.space INITSTACK_SIZE
initstack_end:

# vim: sw=8 ts=8 et
