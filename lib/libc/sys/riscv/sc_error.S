#include <riscv/abi.h>
#include <riscv/asm.h>

ENTRY(__sc_error)
	/* Set call frame and save syscall error. */
	PTR_ADDI sp, sp, -CALLFRAME_SIZ
	PTR_S ra, CALLFRAME_RA(sp)
	REG_S s0, CALLFRAME_S0(sp)
	mv s0, a1

	/* Obtain errno pointer. */
	call __errno

	/* Set errno to syscall error. */
	INT_S s0, (a0)

	/* Return indicating error. */
	REG_LI a0, -1
	REG_L s0, CALLFRAME_S0(sp)
	PTR_L ra, CALLFRAME_RA(sp)
	PTR_ADDI sp, sp, CALLFRAME_SIZ
	ret
END(__sc_error)

# vim: sw=8 ts=8 et
