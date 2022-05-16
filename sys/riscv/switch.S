#include <riscv/asm.h>
#include <riscv/riscvreg.h>

#include "assym.h"

.macro save_ctx sr
	PTR_ADDI	sp, sp, -CTX_SIZE
	.cfi_def_cfa_offset	CTX_SIZE

	SAVE_REG(\sr, CTX_SR);

	SAVE_REG_CFI(ra, CTX_PC);

	SAVE_REG_CFI(s0, CTX_S0);
	SAVE_REG_CFI(s1, CTX_S1);
	SAVE_REG_CFI(s2, CTX_S2);
	SAVE_REG_CFI(s3, CTX_S3);
	SAVE_REG_CFI(s4, CTX_S4);
	SAVE_REG_CFI(s5, CTX_S5);
	SAVE_REG_CFI(s6, CTX_S6);
	SAVE_REG_CFI(s7, CTX_S7);
	SAVE_REG_CFI(s8, CTX_S8);
	SAVE_REG_CFI(s9, CTX_S9);
	SAVE_REG_CFI(s10, CTX_S10);
	SAVE_REG_CFI(s11, CTX_S11);
.endm

.macro load_ctx tmp
	LOAD_REG(ra, CTX_PC);

	LOAD_REG(s0, CTX_S0);
	LOAD_REG(s1, CTX_S1);
	LOAD_REG(s2, CTX_S2);
	LOAD_REG(s3, CTX_S3);
	LOAD_REG(s4, CTX_S4);
	LOAD_REG(s5, CTX_S5);
	LOAD_REG(s6, CTX_S6);
	LOAD_REG(s7, CTX_S7);
	LOAD_REG(s8, CTX_S8);
	LOAD_REG(s9, CTX_S9);
	LOAD_REG(s10, CTX_S10);
	LOAD_REG(s11, CTX_S11);

	LOAD_CSR(sstatus, CTX_SR, \tmp);

	PTR_ADDI	sp, sp, CTX_SIZE
	.cfi_def_cfa_offset	0
.endm

#if FPU
.macro save_fpu_ctx dst, tmp
	/*
	 * Enable FPU usage in supervisor mode,
	 * so we can access registers.
	 */
	ENABLE_SV_FPU(\tmp

	/* Store registers. */
	SAVE_FCSR(\dst, \tmp);
	SAVE_FPU_REG(f0, FPU_CTX_F0, \dst);
	SAVE_FPU_REG(f1, FPU_CTX_F1, \dst);
	SAVE_FPU_REG(f2, FPU_CTX_F2, \dst);
	SAVE_FPU_REG(f3, FPU_CTX_F3, \dst);
	SAVE_FPU_REG(f4, FPU_CTX_F4, \dst);
	SAVE_FPU_REG(f5, FPU_CTX_F5, \dst);
	SAVE_FPU_REG(f6, FPU_CTX_F6, \dst);
	SAVE_FPU_REG(f7, FPU_CTX_F7, \dst);
	SAVE_FPU_REG(f8, FPU_CTX_F8, \dst);
	SAVE_FPU_REG(f9, FPU_CTX_F9, \dst);
	SAVE_FPU_REG(f10, FPU_CTX_F10, \dst);
	SAVE_FPU_REG(f11, FPU_CTX_F11, \dst);
	SAVE_FPU_REG(f12, FPU_CTX_F12, \dst);
	SAVE_FPU_REG(f13, FPU_CTX_F13, \dst);
	SAVE_FPU_REG(f14, FPU_CTX_F14, \dst);
	SAVE_FPU_REG(f15, FPU_CTX_F15, \dst);
	SAVE_FPU_REG(f16, FPU_CTX_F16, \dst);
	SAVE_FPU_REG(f17, FPU_CTX_F17, \dst);
	SAVE_FPU_REG(f18, FPU_CTX_F18, \dst);
	SAVE_FPU_REG(f19, FPU_CTX_F19, \dst);
	SAVE_FPU_REG(f20, FPU_CTX_F20, \dst);
	SAVE_FPU_REG(f21, FPU_CTX_F21, \dst);
	SAVE_FPU_REG(f22, FPU_CTX_F22, \dst);
	SAVE_FPU_REG(f23, FPU_CTX_F23, \dst);
	SAVE_FPU_REG(f24, FPU_CTX_F24, \dst);
	SAVE_FPU_REG(f25, FPU_CTX_F25, \dst);
	SAVE_FPU_REG(f26, FPU_CTX_F26, \dst);
	SAVE_FPU_REG(f27, FPU_CTX_F27, \dst);
	SAVE_FPU_REG(f28, FPU_CTX_F28, \dst);
	SAVE_FPU_REG(f29, FPU_CTX_F29, \dst);
	SAVE_FPU_REG(f30, FPU_CTX_F30, \dst);
	SAVE_FPU_REG(f31, FPU_CTX_F31, \dst);

	DISABLE_SV_FPU(\tmp);
.endm
#endif /* FPU */

/*
 * long ctx_switch(thread_t *from, thread_t *to)
 */
ENTRY(ctx_switch)
	/* `ctx_switch` must be called with interrupts disabled. */
	csrr	t0, sstatus
	li	t1, SSTATUS_SIE
	and	t1, t0, t1
	bnez	t1, halt
	
	/* Save context of `from` thread. */
	save_ctx t0
	PTR_S	sp, TD_KCTX(a0)

#if FPU
	/* Read private thread flags. */
	INT_L	t1, TD_PFLAGS(a0)

	/* If FPU isn't used or FPE context has already been saved,
	 * then skip the saving. */
	li	t2, TDP_FPUINUSE|TDP_FPUCTXSAVED
	and	t2, t1, t2
	li	t3, TDP_FPUINUSE
	bne	t2, t3, skip_fpu_save

	/* Get UCTX pointer for `from` thread. */
	PTR_L	t4, TD_UCTX(a0)

	/* Save the FPE context only if it's dirty. */
	li	t2, SSTATUS_FS_MASK
	and	t3, t0, t2
	li	t2, SSTATUS_FS_DIRTY
	bne	t3, t2, set_ctxsaved

	/* Mark FPE state clean. */
	li	t2, ~SSTATUS_FS_MASK
	and	t0, t0, t2
	li	t2, SSTATUS_FS_CLEAN
	or	t0, t0, t2
	INT_S	t0, CTX_SR(t4)

set_ctxsaved:
	/* Set TDP_FPUCTXSAVED flag. */
	li	t0, TDP_FPUCTXSAVED
	or	t1, t1, t0
	INT_S	t1, TD_PFLAGS(a0)

	/* Save FP regs. */
	save_fpu_ctx t4, t0 

skip_fpu_save:
#endif /* FPU */
	/* Switch stack pointer to `to` thread. */
	PTR_L	sp, TD_KCTX(a1)

	/* Update `curthread` pointer to reference `to` thread. */
	PTR_S	a1, PCPU_CURTHREAD(tp)

	/* Switch address space if necessary. */
	mv	a0, a1
	call	vm_map_switch

	/* Restore `to` thread context. */
	load_ctx t0

	ret
halt:
	wfi
	j	halt
END(ctx_switch)

# vim: sw=8 ts=8 et
