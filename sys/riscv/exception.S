#include <riscv/asm.h>

#include "assym.h"

.macro save_reg reg, offset
	REG_S \reg, (\offset)(sp)
.endm

.macro save_csr csr, offset
	csrr t0, \csr
	save_reg t0, \offset
.endm

.macro load_reg reg, offset
	REG_L \reg, (\offset)(sp)
.endm

.macro load_csr csr, offset
	load_reg t0, \offset
	csrw \csr, t0
.endm

/* 
 * MODE:
 *  - 0: we came from userspace
 *  - 1: we came from kernelspace
 */
.macro save_ctx mode
	PTR_ADDI sp, sp, -CTX_SIZE

	save_reg ra, CTX_RA

.if \mode == 0
	/* Load kernel's global pointer. */
	LOAD_GP()

	/* Load pcpu pointer. */
	save_reg tp, CTX_TP
	PTR_LA tp, _pcpu_data
.endif

	save_reg t0, CTX_T0
	save_reg t1, CTX_T1
	save_reg t2, CTX_T2
	save_reg t3, CTX_T3
	save_reg t4, CTX_T4
	save_reg t5, CTX_T5
	save_reg t6, CTX_T6

	save_reg s0, CTX_S0
	save_reg s1, CTX_S1
	save_reg s2, CTX_S2
	save_reg s3, CTX_S3
	save_reg s4, CTX_S4
	save_reg s5, CTX_S5
	save_reg s6, CTX_S6
	save_reg s7, CTX_S7
	save_reg s8, CTX_S8
	save_reg s9, CTX_S9
	save_reg s10, CTX_S10
	save_reg s11, CTX_S11

	save_reg a0, CTX_A0
	save_reg a1, CTX_A1
	save_reg a2, CTX_A2
	save_reg a3, CTX_A3
	save_reg a4, CTX_A4
	save_reg a5, CTX_A5
	save_reg a6, CTX_A6
	save_reg a7, CTX_A7

	/* Store stack pointer. */
.if \mode == 1
	REG_LI t1, CTX_SIZE
	REG_ADD t0, sp, t1
	save_reg t0, CTX_SP
.else
	/* SSCRATCH should reflect we're in supervisor mode. */
	REG_LI t0, 0
	csrrw t0, sscratch, t0
	save_reg t0, CTX_SP
.endif

	save_csr sepc, CTX_PC
	save_csr sstatus, CTX_SR
	save_csr stval, CTX_TVAL
	save_csr scause, CTX_CAUSE
.endm

/* 
 * MODE:
 *  - 0: we came from userspace
 *  - 1: we came from kernelspace
 */
.macro load_ctx mode
	/* Restore status register.
	 * NOTE: the SR has interrupts disabled. */
	load_csr sstatus, CTX_SR
	load_csr sepc, CTX_PC

	load_reg ra, CTX_RA

.if \mode == 0
	/* Load user's SP to SSCRATCH. */
	load_csr sscratch, CTX_SP

	/* Restore user's TP and GP. */
	load_reg tp, CTX_TP
	load_reg gp, CTX_GP
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

	PTR_ADDI sp, sp, CTX_SIZE
.endm

	.global user_exc_leave
	.global kern_exc_leave

ENTRY(cpu_exception_handler)
	csrrw sp, sscratch, sp
	beqz sp, 1f
	/* User mode detected. */
	j cpu_exception_handler_user
1:
	/* Supervisor mode detected. */
	csrrw sp, sscratch, sp
	j cpu_exception_handler_supervisor
END(cpu_exception_handler)

ENTRY(cpu_exception_handler_supervisor)
	save_ctx 1
	mv a0, sp
	call trap_handler
kern_exc_leave:
	load_ctx 1
	sret
END(cpu_exception_handler_supervisor)

ENTRY(cpu_exception_handler_user)
	# Not implemented!
user_exc_leave:
	j .
END(cpu_exception_handler_user)

# vim: sw=8 ts=8 et
