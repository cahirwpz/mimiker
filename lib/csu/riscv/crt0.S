#include <riscv/abi.h>
#include <riscv/asm.h>

/*
 * This is the starting procedude for user ELF images, used as their
 * entry point. It takes care of preparing argc, argv and envp for main().
 * For details about environment in which this procedure starts, refer
 * to `exec_args_copyout` in sys/kern/exec.c.
 */
ENTRY(_start)
	/*
	 * Adhere to calling convention wrt return address so that GDB can
	 * properly recognize stack frames.
	 * However, store zero as RA, to mark stack bottom.
	 */
	PTR_ADDI sp, sp, -CALLFRAME_SIZ
	PTR_S zero, CALLFRAME_RA(sp)
	PTR_S sp, CALLFRAME_SP(sp)

	/* Prepare global pointer. */
	LOAD_GP()

	/* Grab argc from below stack. */
	LONG_L a0, CALLFRAME_SIZ(sp)

	/* Prepare argv pointing ar argument vector below stack. */
	PTR_ADDI a1, sp, CALLFRAME_SIZ + SZREG

	/* Prepare envp, it starts at argv + argc + 1. */
	LONG_ADDI t0, a0, 1	/* argv is NULL terminated */
	LONG_SLLI t0, t0, LONG_SCALESHIFT
	PTR_ADD a2, a1, t0

	/* Jump to start in crt0-common.c. */
	j _C_LABEL(___start)
END(_start)

# vim: sw=8 ts=8 et
