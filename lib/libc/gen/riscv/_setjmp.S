#include <riscv/asm.h>

#include "riscv/assym.h"

/*
 * C library -- _setjmp, _longjmp
 *
 *	_longjmp(a,v)
 * will generate a "return(v?v:1)" from the last call to
 *	_setjmp(a)
 * by restoring registers from the stack.
 * The previous signal state is NOT restored.
 */

ENTRY(_setjmp)
	REG_S ra, UC_GREGS_PC(a0)
	REG_S sp, UC_GREGS_SP(a0)
	REG_S gp, UC_GREGS_GP(a0)
	REG_S tp, UC_GREGS_TP(a0)
	REG_S s0, UC_GREGS_S0(a0)
	REG_S s1, UC_GREGS_S1(a0)
	REG_S s2, UC_GREGS_S2(a0)
	REG_S s3, UC_GREGS_S3(a0)
	REG_S s4, UC_GREGS_S4(a0)
	REG_S s5, UC_GREGS_S5(a0)
	REG_S s6, UC_GREGS_S6(a0)
	REG_S s7, UC_GREGS_S7(a0)
	REG_S s8, UC_GREGS_S8(a0)
	REG_S s9, UC_GREGS_S9(a0)
	REG_S s10, UC_GREGS_S10(a0)
	REG_S s11, UC_GREGS_S11(a0)

	INT_L t0, UC_FLAGS(a0)
	li t1, _UC_CPU
	or t0, t0, t1

#ifndef __riscv_float_abi_soft
	frcsr t1
	REG_S t1, UC_FPREGS_FCSR(a0)
	FP_S fs0, UC_FPREGS_FS0(a0)
	FP_S fs1, UC_FPREGS_FS1(a0)
	FP_S fs2, UC_FPREGS_FS2(a0)
	FP_S fs3, UC_FPREGS_FS3(a0)
	FP_S fs4, UC_FPREGS_FS4(a0)
	FP_S fs5, UC_FPREGS_FS5(a0)
	FP_S fs6, UC_FPREGS_FS6(a0)
	FP_S fs7, UC_FPREGS_FS7(a0)
	FP_S fs8, UC_FPREGS_FS8(a0)
	FP_S fs9, UC_FPREGS_FS9(a0)
	FP_S fs10, UC_FPREGS_FS10(a0)
	FP_S fs11, UC_FPREGS_FS11(a0)

	li t1, _UC_FPU
	or t0, t0, t1
#endif
	INT_S t0, UC_FLAGS(a0)

	mv a0, zero
	ret
END(_setjmp)

ENTRY(_longjmp)
	REG_L ra, UC_GREGS_PC(a0)
	REG_L sp, UC_GREGS_SP(a0)
	REG_L gp, UC_GREGS_GP(a0)
	REG_L tp, UC_GREGS_TP(a0)
	REG_L s0, UC_GREGS_S0(a0)
	REG_L s1, UC_GREGS_S1(a0)
	REG_L s2, UC_GREGS_S2(a0)
	REG_L s3, UC_GREGS_S3(a0)
	REG_L s4, UC_GREGS_S4(a0)
	REG_L s5, UC_GREGS_S5(a0)
	REG_L s6, UC_GREGS_S6(a0)
	REG_L s7, UC_GREGS_S7(a0)
	REG_L s8, UC_GREGS_S8(a0)
	REG_L s9, UC_GREGS_S9(a0)
	REG_L s10, UC_GREGS_S10(a0)
	REG_L s11, UC_GREGS_S11(a0)

#ifndef __riscv_float_abi_soft
	REG_L t0, UC_FPREGS_FCSR(a0)
	fscsr t0
	FP_L fs0, UC_FPREGS_FS0(a0)
	FP_L fs1, UC_FPREGS_FS1(a0)
	FP_L fs2, UC_FPREGS_FS2(a0)
	FP_L fs3, UC_FPREGS_FS3(a0)
	FP_L fs4, UC_FPREGS_FS4(a0)
	FP_L fs5, UC_FPREGS_FS5(a0)
	FP_L fs6, UC_FPREGS_FS6(a0)
	FP_L fs7, UC_FPREGS_FS7(a0)
	FP_L fs8, UC_FPREGS_FS8(a0)
	FP_L fs9, UC_FPREGS_FS9(a0)
	FP_L fs10, UC_FPREGS_FS10(a0)
	FP_L fs11, UC_FPREGS_FS11(a0)
#endif

	REG_LI a0, 1	/* default return value */
	beqz a1, 0f
	mv a0, a1
0:	ret
END(_longjmp)

# vim: sw=8 ts=8 et
