#include <sys/errno.h>
#include <riscv/asm.h>

ENTRY(__sc_missing)
	/*
	 * Behave as if the syscall returned ENOSYS.
	 */
	REG_LI a0, -1
	REG_LI a1, ENOSYS
	j _C_LABEL(__sc_error)
END(__sc_missing)

# vim: sw=8 ts=8 et
