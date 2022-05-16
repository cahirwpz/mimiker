#include <riscv/asm.h>
#include <riscv/riscvreg.h>

#include "assym.h"

/* 
 * MODE:
 *  - 0: we came from userspace
 *  - 1: we came from kernelspace
 */
.macro save_ctx mode
.if \mode == 1
	PTR_ADDI	sp, sp, -CTX_SIZE
.endif

	SAVE_REG_CFI(ra, CTX_RA);

.if \mode == 0
	/* Load kernel's global pointer. */
	SAVE_REG_CFI(gp, CTX_GP);
	LOAD_GP()

	/* Load pcpu pointer. */
	SAVE_REG_CFI(tp, CTX_TP);
	PTR_LA	tp, _pcpu_data
.endif

	SAVE_REG_CFI(t0, CTX_T0);
	SAVE_REG_CFI(t1, CTX_T1);
	SAVE_REG_CFI(t2, CTX_T2);
	SAVE_REG_CFI(t3, CTX_T3);
	SAVE_REG_CFI(t4, CTX_T4);
	SAVE_REG_CFI(t5, CTX_T5);
	/* XXX: we don't provide the CFI directive for $t6 as
	 * (GCC 12.1.0, Binutils 2.38, GDB 12.1)  has faulty frame info
	 * implementation which confuses this register with PC. */
	SAVE_REG(t6, CTX_T6);

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

	SAVE_REG_CFI(a0, CTX_A0);
	SAVE_REG_CFI(a1, CTX_A1);
	SAVE_REG_CFI(a2, CTX_A2);
	SAVE_REG_CFI(a3, CTX_A3);
	SAVE_REG_CFI(a4, CTX_A4);
	SAVE_REG_CFI(a5, CTX_A5);
	SAVE_REG_CFI(a6, CTX_A6);
	SAVE_REG_CFI(a7, CTX_A7);

	/* Store stack pointer. */
.if \mode == 1
	REG_LI	t1, CTX_SIZE
	PTR_ADD	t0, sp, t1
.else
	/* SSCRATCH should reflect we're in supervisor mode. */
	REG_LI	t0, 0
	csrrw	t0, sscratch, t0
.endif
	SAVE_REG(t0, CTX_SP);
	.cfi_rel_offset	sp, CTX_SP

	SAVE_CSR(sepc, CTX_PC, t0);
	SAVE_CSR(sstatus, CTX_SR, t0);
	SAVE_CSR(stval, CTX_TVAL, t0);
	SAVE_CSR(scause, CTX_CAUSE, t0);
.endm

/* 
 * MODE:
 *  - 0: we came from userspace
 *  - 1: we came from kernelspace
 */
.macro load_ctx mode
.if \mode == 0
	PTR_L	t0, PCPU_CURTHREAD(tp)
	PTR_L	sp, TD_UCTX(t0)
.endif

	/* Restore status register.
	 * NOTE: the SR has interrupts disabled. */
	LOAD_CSR(sstatus, CTX_SR, t0);
	LOAD_CSR(sepc, CTX_PC, t0);

	LOAD_REG(ra, CTX_RA);

.if \mode == 0
	/* Load user's SP to SSCRATCH. */
	LOAD_CSR(sscratch, CTX_SP, t0);

	/* Restore user's TP and GP. */
	LOAD_REG(gp, CTX_GP);
	LOAD_REG(tp, CTX_TP);
.endif

	LOAD_REG(t0, CTX_T0);
	LOAD_REG(t1, CTX_T1);
	LOAD_REG(t2, CTX_T2);
	LOAD_REG(t3, CTX_T3);
	LOAD_REG(t4, CTX_T4);
	LOAD_REG(t5, CTX_T5);
	LOAD_REG(t6, CTX_T6);

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

	LOAD_REG(a0, CTX_A0);
	LOAD_REG(a1, CTX_A1);
	LOAD_REG(a2, CTX_A2);
	LOAD_REG(a3, CTX_A3);
	LOAD_REG(a4, CTX_A4);
	LOAD_REG(a5, CTX_A5);
	LOAD_REG(a6, CTX_A6);
	LOAD_REG(a7, CTX_A7);

.if \mode == 1
	PTR_ADDI	sp, sp, CTX_SIZE
.endif
.endm

#if FPU
.macro load_fpu_ctx src, tmp
	/*
	 * Enable FPE usage in supervisor mode,
	 * so we can access registers.
	 */
	ENABLE_SV_FPU(\tmp);

	/* Restore registers. */
	LOAD_FCSR(\src, \tmp);

	LOAD_FPU_REG(f0, FPU_CTX_F0, \src);
	LOAD_FPU_REG(f1, FPU_CTX_F1, \src);
	LOAD_FPU_REG(f2, FPU_CTX_F2, \src);
	LOAD_FPU_REG(f3, FPU_CTX_F3, \src);
	LOAD_FPU_REG(f4, FPU_CTX_F4, \src);
	LOAD_FPU_REG(f5, FPU_CTX_F5, \src);
	LOAD_FPU_REG(f6, FPU_CTX_F6, \src);
	LOAD_FPU_REG(f7, FPU_CTX_F7, \src);
	LOAD_FPU_REG(f8, FPU_CTX_F8, \src);
	LOAD_FPU_REG(f9, FPU_CTX_F9, \src);
	LOAD_FPU_REG(f10, FPU_CTX_F10, \src);
	LOAD_FPU_REG(f11, FPU_CTX_F11, \src);
	LOAD_FPU_REG(f12, FPU_CTX_F12, \src);
	LOAD_FPU_REG(f13, FPU_CTX_F13, \src);
	LOAD_FPU_REG(f14, FPU_CTX_F14, \src);
	LOAD_FPU_REG(f15, FPU_CTX_F15, \src);
	LOAD_FPU_REG(f16, FPU_CTX_F16, \src);
	LOAD_FPU_REG(f17, FPU_CTX_F17, \src);
	LOAD_FPU_REG(f18, FPU_CTX_F18, \src);
	LOAD_FPU_REG(f19, FPU_CTX_F19, \src);
	LOAD_FPU_REG(f20, FPU_CTX_F20, \src);
	LOAD_FPU_REG(f21, FPU_CTX_F21, \src);
	LOAD_FPU_REG(f22, FPU_CTX_F22, \src);
	LOAD_FPU_REG(f23, FPU_CTX_F23, \src);
	LOAD_FPU_REG(f24, FPU_CTX_F24, \src);
	LOAD_FPU_REG(f25, FPU_CTX_F25, \src);
	LOAD_FPU_REG(f26, FPU_CTX_F26, \src);
	LOAD_FPU_REG(f27, FPU_CTX_F27, \src);
	LOAD_FPU_REG(f28, FPU_CTX_F28, \src);
	LOAD_FPU_REG(f29, FPU_CTX_F29, \src);
	LOAD_FPU_REG(f30, FPU_CTX_F30, \src);
	LOAD_FPU_REG(f31, FPU_CTX_F31, \src);

	DISABLE_SV_FPU(\tmp);
.endm
#endif /* FPU */

	.global cpu_exception_handler
	.global user_exc_leave
	.global kern_exc_leave

cpu_exception_handler:
	csrrw	sp, sscratch, sp
	beqz	sp, 1f

	/* User mode detected. */
	j	cpu_exception_handler_user

1:
	/* Supervisor mode detected. */
	csrrw	sp, sscratch, sp

ENTRY(cpu_exception_handler_supervisor)
	.cfi_def_cfa	sp, 0
	save_ctx 1
	mv	a0, sp
	call	trap_handler
kern_exc_leave:
	load_ctx 1
	sret
END(cpu_exception_handler_supervisor)

ENTRY(cpu_exception_handler_user)
	.cfi_signal_frame
	.cfi_def_cfa	sp, 0
	save_ctx 0
	.cfi_return_column	t0
	.cfi_rel_offset	t0, CTX_PC
	mv	a0, sp
	call	trap_handler
user_exc_leave:
	/* Disable interrupts. */
	li	t0, SSTATUS_SIE
	csrc	sstatus, t0

#if FPU
	/* Read private thread flags. */
	PTR_L	t0, PCPU_CURTHREAD(tp)
	INT_L	t1, TD_PFLAGS(t0)

	/* Skip FPU restoring if FPE context is not used. */
	li	t2, TDP_FPUINUSE
	and	t2, t1, t2
	beqz	t2, skip_fpu_restore

	/* Restore FPE context iff a context has been saved. */
	li	t2, TDP_FPUCTXSAVED
	and	t2, t1, t2
	beqz	t2, skip_fpu_restore

	/* Clear TDP_FPUCTXSAVED flag. */
	li	t2, ~TDP_FPUCTXSAVED
	and	t1, t1, t2
	INT_S	t1, TD_PFLAGS(t0)

	/* Restore FPE context. */
	PTR_L	t0, TD_UCTX(t0)
	load_fpu_ctx t0, t1

skip_fpu_restore:
#endif
	load_ctx 0
	csrrw	sp, sscratch, sp
	sret
END(cpu_exception_handler_user)

# vim: sw=8 ts=8 et
