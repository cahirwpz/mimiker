#include <riscv/asm.h>

#include "riscv/assym.h"

/*
 * C library -- sigsetjmp, siglongjmp
 *
 *	siglongjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	sigsetjmp(a, savemask)
 * by restoring registers from the stack,
 * and dependent on savemask restores the
 * signal mask.
 */

ENTRY(sigsetjmp)
	bnez a1, 0f

	/* Zero sigmask flag and move to `_setjmp`. */
	INT_S zero, UC_FLAGS(a0)
	j _C_LABEL(_setjmp)

0:
	/* Set sigmask flag and move to `setjmp`. */
	REG_LI a1, _UC_SIGMASK
	INT_S a1, UC_FLAGS(a0)
	j _C_LABEL(setjmp)
END(sigsetjmp)

ENTRY(siglongjmp)
	/* Get sigmask flag. */
	INT_LU t0, UC_FLAGS(a0)
	andi t0, t0, _UC_SIGMASK
	bnez t0, 0f

	/* Signal mask in not stored. Move to `_longjmp`. */
	j _C_LABEL(_longjmp)

0:
	/* Signal mask is stored. Move to `longjmp`. */
	j _C_LABEL(longjmp)
END(siglongjmp)

# vim: sw=8 ts=8 et
