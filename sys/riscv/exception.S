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

.macro save_csr csr, offset, tmp
	csrr \tmp, \csr
	save_reg \tmp, \offset
.endm

.macro load_reg reg, offset
	REG_L \reg, (\offset)(sp)
.endm

.macro load_csr csr, offset, tmp
	load_reg \tmp, \offset
	csrw \csr, \tmp
.endm

/* 
 * MODE:
 *  - 0: we came from userspace
 *  - 1: we came from kernelspace
 */
.macro save_ctx mode
.if \mode == 1
	PTR_ADDI sp, sp, -CTX_SIZE
.endif
	.cfi_def_cfa sp, 0

	save_reg_cfi ra, CTX_RA

.if \mode == 0
	/* Load kernel's global pointer. */
	save_reg_cfi gp, CTX_GP
	LOAD_GP()

	/* Load pcpu pointer. */
	save_reg_cfi tp, CTX_TP
	PTR_LA tp, _pcpu_data
.endif

	save_reg_cfi t0, CTX_T0
	save_reg_cfi t1, CTX_T1
	save_reg_cfi t2, CTX_T2
	save_reg_cfi t3, CTX_T3
	save_reg_cfi t4, CTX_T4
	save_reg_cfi t5, CTX_T5
	save_reg_cfi t6, CTX_T6

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

	save_reg_cfi a0, CTX_A0
	save_reg_cfi a1, CTX_A1
	save_reg_cfi a2, CTX_A2
	save_reg_cfi a3, CTX_A3
	save_reg_cfi a4, CTX_A4
	save_reg_cfi a5, CTX_A5
	save_reg_cfi a6, CTX_A6
	save_reg_cfi a7, CTX_A7

	/* Store stack pointer. */
.if \mode == 1
	REG_LI t1, CTX_SIZE
	PTR_ADD t0, sp, t1
.else
	/* SSCRATCH should reflect we're in supervisor mode. */
	REG_LI t0, 0
	csrrw t0, sscratch, t0
.endif
	save_reg t0, CTX_SP
	.cfi_rel_offset sp, CTX_SP

	save_csr sepc, CTX_PC, t0
	save_csr sstatus, CTX_SR, t0
	save_csr stval, CTX_TVAL, t0
	save_csr scause, CTX_CAUSE, t0
.endm

/* 
 * MODE:
 *  - 0: we came from userspace
 *  - 1: we came from kernelspace
 */
.macro load_ctx mode
.if \mode == 0
	PTR_L t0, PCPU_CURTHREAD(tp)
	PTR_L sp, TD_UCTX(t0)
.endif

	/* Restore status register.
	 * NOTE: the SR has interrupts disabled. */
	load_csr sstatus, CTX_SR, t0
	load_csr sepc, CTX_PC, t0

	load_reg ra, CTX_RA

.if \mode == 0
	/* Load user's SP to SSCRATCH. */
	load_csr sscratch, CTX_SP, t0

	/* Restore user's TP and GP. */
	load_reg gp, CTX_GP
	load_reg tp, CTX_TP
.endif

	load_reg t0, CTX_T0
	load_reg t1, CTX_T1
	load_reg t2, CTX_T2
	load_reg t3, CTX_T3
	load_reg t4, CTX_T4
	load_reg t5, CTX_T5
	load_reg t6, CTX_T6

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

	load_reg a0, CTX_A0
	load_reg a1, CTX_A1
	load_reg a2, CTX_A2
	load_reg a3, CTX_A3
	load_reg a4, CTX_A4
	load_reg a5, CTX_A5
	load_reg a6, CTX_A6
	load_reg a7, CTX_A7

.if \mode == 1
	PTR_ADDI sp, sp, CTX_SIZE
.endif
.endm

#if FPE
.macro enable_sv_fpu tmp
	li \tmp, SSTATUS_FS_INITIAL
	csrs sstatus, \tmp
.endm

.macro disable_sv_fpu tmp
	li \tmp, SSTATUS_FS_MASK
	csrc sstatus, \tmp
.endm

.macro load_fpu_reg reg, offset, src
	FP_L \reg, (\offset)(\src)
.endm

.macro load_fcsr src, tmp
	REG_L \tmp, (FPU_CTX_FCSR)(\src)
	fscsr \tmp
.endm

.macro load_fpu_ctx src, tmp
	/*
	 * Enable FPE usage in supervisor mode,
	 * so we can access registers.
	 */
	enable_sv_fpu \tmp

	/* Restore registers. */
	load_fcsr \src, \tmp

	load_fpu_reg f0, FPU_CTX_F0, \src
	load_fpu_reg f1, FPU_CTX_F1, \src
	load_fpu_reg f2, FPU_CTX_F2, \src
	load_fpu_reg f3, FPU_CTX_F3, \src
	load_fpu_reg f4, FPU_CTX_F4, \src
	load_fpu_reg f5, FPU_CTX_F5, \src
	load_fpu_reg f6, FPU_CTX_F6, \src
	load_fpu_reg f7, FPU_CTX_F7, \src
	load_fpu_reg f8, FPU_CTX_F8, \src
	load_fpu_reg f9, FPU_CTX_F9, \src
	load_fpu_reg f10, FPU_CTX_F10, \src
	load_fpu_reg f11, FPU_CTX_F11, \src
	load_fpu_reg f12, FPU_CTX_F12, \src
	load_fpu_reg f13, FPU_CTX_F13, \src
	load_fpu_reg f14, FPU_CTX_F14, \src
	load_fpu_reg f15, FPU_CTX_F15, \src
	load_fpu_reg f16, FPU_CTX_F16, \src
	load_fpu_reg f17, FPU_CTX_F17, \src
	load_fpu_reg f18, FPU_CTX_F18, \src
	load_fpu_reg f19, FPU_CTX_F19, \src
	load_fpu_reg f20, FPU_CTX_F20, \src
	load_fpu_reg f21, FPU_CTX_F21, \src
	load_fpu_reg f22, FPU_CTX_F22, \src
	load_fpu_reg f23, FPU_CTX_F23, \src
	load_fpu_reg f24, FPU_CTX_F24, \src
	load_fpu_reg f25, FPU_CTX_F25, \src
	load_fpu_reg f26, FPU_CTX_F26, \src
	load_fpu_reg f27, FPU_CTX_F27, \src
	load_fpu_reg f28, FPU_CTX_F28, \src
	load_fpu_reg f29, FPU_CTX_F29, \src
	load_fpu_reg f30, FPU_CTX_F30, \src
	load_fpu_reg f31, FPU_CTX_F31, \src

	disable_sv_fpu \tmp
.endm
#endif

	.global cpu_exception_handler
	.global user_exc_leave
	.global kern_exc_leave

cpu_exception_handler:
	csrrw sp, sscratch, sp
	beqz sp, 1f

	/* User mode detected. */
	j cpu_exception_handler_user

1:
	/* Supervisor mode detected. */
	csrrw sp, sscratch, sp

ENTRY(cpu_exception_handler_supervisor)
	save_ctx 1
	mv a0, sp
	call trap_handler
kern_exc_leave:
	load_ctx 1
	sret
END(cpu_exception_handler_supervisor)

ENTRY(cpu_exception_handler_user)
	.cfi_signal_frame
	save_ctx 0
	mv a0, sp
	call trap_handler	
user_exc_leave:
	/* Disable interrupts. */
	li t0, SSTATUS_SIE
	csrc sstatus, t0

#if FPE
	/* Read private thread flags. */
	PTR_L t0, PCPU_CURTHREAD(tp)
	INT_L t1, TD_PFLAGS(t0)

	/* Skip FPU restoring if FPE context is not used. */
	li t2, TDP_FPUINUSE
	and t2, t1, t2
	beqz t2, skip_fpu_restore

	/* Restore FPE context iff a context has been saved. */
	li t2, TDP_FPUCTXSAVED
	and t2, t1, t2
	beqz t2, skip_fpu_restore

	/* Clear TDP_FPUCTXSAVED flag. */
	li t2, ~TDP_FPUCTXSAVED
	and t1, t1, t2
	INT_S t1, TD_PFLAGS(t0)

	/* Restore FPE context. */
	PTR_L t0, TD_UCTX(t0)
	load_fpu_ctx t0, t1

skip_fpu_restore:
#endif
	load_ctx 0
	csrrw sp, sscratch, sp
	sret
END(cpu_exception_handler_user)

# vim: sw=8 ts=8 et
