#include <riscv/asm.h>
#include <riscv/riscvreg.h>

#include "assym.h"

.macro save_reg reg, offset
	REG_S \reg, (\offset)(sp)
.endm

.macro save_reg_cfi reg, offset
	save_reg \reg, \offset
	.cfi_rel_offset \reg, \offset
.endm

.macro load_reg reg, offset
	REG_L \reg, (\offset)(sp)
.endm

.macro load_csr csr, offset, tmp
	load_reg \tmp, \offset
	csrw \csr, \tmp
.endm

.macro save_ctx sr
	PTR_ADDI sp, sp, -CTX_SIZE
	.cfi_def_cfa_offset CTX_SIZE

	save_reg \sr, CTX_SR

	save_reg_cfi ra, CTX_PC

	save_reg_cfi s0, CTX_S0
	save_reg_cfi s1, CTX_S1
	save_reg_cfi s2, CTX_S2
	save_reg_cfi s3, CTX_S3
	save_reg_cfi s4, CTX_S4
	save_reg_cfi s5, CTX_S5
	save_reg_cfi s6, CTX_S6
	save_reg_cfi s7, CTX_S7
	save_reg_cfi s8, CTX_S8
	save_reg_cfi s9, CTX_S9
	save_reg_cfi s10, CTX_S10
	save_reg_cfi s11, CTX_S11

	/* `ctx_switch` returns a long value, initialize it to 0. */
	save_reg zero, CTX_RV
.endm

.macro load_ctx tmp
	load_reg ra, CTX_PC

	load_reg s0, CTX_S0
	load_reg s1, CTX_S1
	load_reg s2, CTX_S2
	load_reg s3, CTX_S3
	load_reg s4, CTX_S4
	load_reg s5, CTX_S5
	load_reg s6, CTX_S6
	load_reg s7, CTX_S7
	load_reg s8, CTX_S8
	load_reg s9, CTX_S9
	load_reg s10, CTX_S10
	load_reg s11, CTX_S11

	load_reg a0, CTX_RV

	load_csr sstatus, CTX_SR, \tmp

	PTR_ADDI sp, sp, CTX_SIZE
	.cfi_def_cfa_offset 0
.endm

#if FPU
.macro enable_sv_fpu tmp
	li \tmp, SSTATUS_FS_INITIAL
	csrs sstatus, \tmp
.endm

.macro disable_sv_fpu tmp
	li \tmp, SSTATUS_FS_MASK
	csrc sstatus, \tmp
.endm

.macro save_fpu_reg reg, offset, dst
	FP_S \reg, (\offset)(\dst)
.endm

.macro save_fcsr dst, tmp
	frcsr \tmp
	REG_S \tmp, (FPU_CTX_FCSR)(\dst)
.endm

.macro save_fpu_ctx dst, tmp
	/*
	 * Enable FPU usage in supervisor mode,
	 * so we can access registers.
	 */
	enable_sv_fpu \tmp

	/* Store registers. */
	save_fcsr \dst, \tmp
	save_fpu_reg f0, FPU_CTX_F0, \dst
	save_fpu_reg f1, FPU_CTX_F1, \dst
	save_fpu_reg f2, FPU_CTX_F2, \dst
	save_fpu_reg f3, FPU_CTX_F3, \dst
	save_fpu_reg f4, FPU_CTX_F4, \dst
	save_fpu_reg f5, FPU_CTX_F5, \dst
	save_fpu_reg f6, FPU_CTX_F6, \dst
	save_fpu_reg f7, FPU_CTX_F7, \dst
	save_fpu_reg f8, FPU_CTX_F8, \dst
	save_fpu_reg f9, FPU_CTX_F9, \dst
	save_fpu_reg f10, FPU_CTX_F10, \dst
	save_fpu_reg f11, FPU_CTX_F11, \dst
	save_fpu_reg f12, FPU_CTX_F12, \dst
	save_fpu_reg f13, FPU_CTX_F13, \dst
	save_fpu_reg f14, FPU_CTX_F14, \dst
	save_fpu_reg f15, FPU_CTX_F15, \dst
	save_fpu_reg f16, FPU_CTX_F16, \dst
	save_fpu_reg f17, FPU_CTX_F17, \dst
	save_fpu_reg f18, FPU_CTX_F18, \dst
	save_fpu_reg f19, FPU_CTX_F19, \dst
	save_fpu_reg f20, FPU_CTX_F20, \dst
	save_fpu_reg f21, FPU_CTX_F21, \dst
	save_fpu_reg f22, FPU_CTX_F22, \dst
	save_fpu_reg f23, FPU_CTX_F23, \dst
	save_fpu_reg f24, FPU_CTX_F24, \dst
	save_fpu_reg f25, FPU_CTX_F25, \dst
	save_fpu_reg f26, FPU_CTX_F26, \dst
	save_fpu_reg f27, FPU_CTX_F27, \dst
	save_fpu_reg f28, FPU_CTX_F28, \dst
	save_fpu_reg f29, FPU_CTX_F29, \dst
	save_fpu_reg f30, FPU_CTX_F30, \dst
	save_fpu_reg f31, FPU_CTX_F31, \dst

	disable_sv_fpu \tmp
.endm
#endif

/*
 * long ctx_switch(thread_t *from, thread_t *to)
 */
ENTRY(ctx_switch)
	/* `ctx_switch` must be called with interrupts disabled. */
	csrr t0, sstatus
	li t1, SSTATUS_SIE
	and t1, t0, t1
	bnez t1, halt
	
	/* Save context of `from` thread. */
	save_ctx t0
	PTR_S sp, TD_KCTX(a0)

#if FPU
	/* Read private thread flags. */
	INT_L t1, TD_PFLAGS(a0)

	/* If FPU isn't used or FPE context has already been saved,
	 * then skip the saving. */
	li t2, TDP_FPUINUSE|TDP_FPUCTXSAVED
	and t2, t1, t2
	li t3, TDP_FPUINUSE
	bne t2, t3, skip_fpu_save

	/* Get UCTX pointer for `from` thread. */
	PTR_L t4, TD_UCTX(a0)

	/* Save the FPE context only if it's dirty. */
	li t2, SSTATUS_FS_MASK
	and t3, t0, t2
	li t2, SSTATUS_FS_DIRTY
	bne t3, t2, set_ctxsaved

	/* Mark FPE state clean. */
	li t2, ~SSTATUS_FS_MASK
	and t0, t0, t2
	li t2, SSTATUS_FS_CLEAN
	or t0, t0, t2
	INT_S t0, CTX_SR(t4)

set_ctxsaved:
	/* Set TDP_FPUCTXSAVED flag. */
	li t0, TDP_FPUCTXSAVED
	or t1, t1, t0
	INT_S t1, TD_PFLAGS(a0)

	/* Save FP regs. */
	save_fpu_ctx t4, t0 

skip_fpu_save:
#endif
	/* Switch stack pointer to `to` thread. */
	PTR_L sp, TD_KCTX(a1)

	/* Update `curthread` pointer to reference `to` thread. */
	PTR_S a1, PCPU_CURTHREAD(tp)

	/* Switch address space if necessary. */
	mv a0, a1
	call vm_map_switch

	/* Restore `to` thread context. */
	load_ctx t0

	ret
halt:
	wfi
	j halt
END(ctx_switch)

# vim: sw=8 ts=8 et
