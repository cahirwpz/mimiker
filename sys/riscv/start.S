#include <riscv/abi.h>
#include <riscv/asm.h>
#include <riscv/vm_param.h>

#define INIT_STACK_SIZE 512

	.option nopic
	.section .boot,"ax",@progbits

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
	PTR_L	gp, _global_pointer

	/*
	 * NOTE: In an actual SMP system, we could have multiple harts
	 * entering the kernel simultaneously. In such case,
	 * we would have to choose the hart to perform the global startup.
	 * FTTB, we assume a single hart entering the kernel.
	 */

	/* Move to the initial stack. */
	PTR_LA	sp, _init_stack_end

	mv	a0, a1
	tail	riscv_init
halt:
	wfi
	j	halt
_END(_start)

	.section .boot.data,"aw",@progbits
_global_pointer:
#if __riscv_xlen == 32
	.long	__global_pointer$
#else
	.quad	__global_pointer$
#endif

	.section .boot.bss,"aw",@nobits

	.align STACK_ALIGN
_init_stack:
	.space INIT_STACK_SIZE
_init_stack_end:

# vim: sw=8 ts=8 et
