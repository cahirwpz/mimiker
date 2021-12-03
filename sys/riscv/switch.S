#include <riscv/asm.h>
#include <riscv/riscvreg.h>

#include "assym.h"

.macro save_reg reg, offset
	REG_S \reg, (\offset)(sp)
.endm

.macro load_reg reg, offset
	REG_L \reg, (\offset)(sp)
.endm

.macro load_csr csr, offset
	load_reg t0, \offset
	csrw \csr, t0
.endm

.macro save_ctx sr
	PTR_ADDI sp, sp, -CTX_SIZE

	save_reg \sr, CTX_SR

	save_reg ra, CTX_PC

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

	/* `ctx_switch` returns a long value, initialize it to 0. */
	save_reg zero, CTX_RV
.endm

.macro load_ctx
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

	PTR_ADDI sp, sp, CTX_SIZE
.endm

/*
 * long ctx_switch(thread_t *from, thread_t *to)
 */
ENTRY(ctx_switch)
	/* `ctx_switch` must be called with interrupts disabled. */
	csrr t0, sstatus
	andi t1, t0, SSTATUS_SIE
	bnez t1, halt
	
	/* Save context of `from` thread. */
	save_ctx t0
	PTR_S sp, TD_KCTX(a0)

	/* Switch stack pointer to `to` thread. */
	PTR_L sp, TD_KCTX(a1)

	/* Update `curthread` pointer to reference `to` thread. */
	PTR_S a1, PCPU_CURTHREAD(tp)

	/* Switch address space if necessary. */
	mv a0, a1
	call vm_map_switch

	/* Restore `to` thread context. */
	load_ctx

	ret
halt:
	wfi
	j halt
END(ctx_switch)
