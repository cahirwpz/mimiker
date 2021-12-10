#include <sys/errno.h>
#include <riscv/asm.h>
#include <riscv/riscvreg.h>

#include "assym.h"

/*
 * NOTE: RISC-V uses generic implementation of libc string routines,
 * thereby cannot make any assumptions regarding its usage of registers
 * nor stack.
 */

.macro save_reg reg, offset
	REG_S \reg, (\offset)(sp)
.endm

.macro load_reg reg, offset
	REG_L \reg, (\offset)(sp)
.endm

.macro ctx_save
	PTR_ADDI sp, sp, -CTX_SIZE

	save_reg ra, CTX_RA

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

	PTR_S sp, PCPU_CPY_CTX(tp)
.endm

.macro ctx_load
	PTR_L sp, PCPU_CPY_CTX(tp)

	load_reg ra, CTX_RA

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

	PTR_ADDI sp, sp, CTX_SIZE
.endm

/*
 * Enable permit Supervisor User Memory access (SUM).
 */
.macro sum_enable tmp
	REG_LI \tmp, SSTATUS_SUM
	csrs sstatus, \tmp
.endm

/*
 * Disable permit Supervisor User Memory access (SUM).
 */
.macro sum_disable tmp
	REG_LI \tmp, SSTATUS_SUM
	csrc sstatus, \tmp
.endm

.macro onfault_set tmp, td, func
	PTR_L \td, PCPU_CURTHREAD(tp)
	PTR_LA \tmp, \func
	PTR_S \tmp, TD_ONFAULT(\td)
	sum_enable \tmp
.endm

.macro onfault_set_and_save tmp, td, func
	ctx_save
	onfault_set \tmp, \td, \func
.endm

.macro onfault_clr tmp, td
	sum_disable \tmp
	PTR_L \td, PCPU_CURTHREAD(tp)
	PTR_S zero, TD_ONFAULT(tp)
.endm

.macro onfault_clr_and_load tmp, td
	onfault_clr \tmp, \td
	ctx_load
.endm

/*
 * int copyinstr(const void *udaddr, void *kaddr, size_t len, size_t *lencopied)
 *
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 */
ENTRY(copyinstr)
	/* len > 0 */
	bnez a2, 0f
	mv a0, zero
	ret

0:	
	/* (uintptr_t)udaddr < (uintptr_t)(udaddr + len) */
	PTR_ADD t0, a0, a2
	bgeu a0, t0, reterr

	/* (uintptr_t)(udaddr + len) <= USER_SPACE_END */
	REG_LI t1, USER_SPACE_END
	bgtu t0, t1, reterr

	onfault_set_and_save t0, t1, copyerr
	call copystr
	onfault_clr_and_load t0, t1

	ret
END(copyinstr)

/*
 * int copyin(const void *udaddr, void *kaddr, size_t len)
 *
 * Copy specified amount of data from user space into the kernel.
 */
ENTRY(copyin)
	/* len > 0 */
	beqz a2, 0f

	/* (uintptr_t)udaddr < (uintptr_t)(udaddr + len) */
	PTR_ADD t0, a0, a2
	bgeu a0, t0, reterr

	/* (uintptr_t)(udaddr + len) <= USER_SPACE_END */
	REG_LI t1, USER_SPACE_END
	bgtu t0, t1, reterr

	onfault_set_and_save t0, t1, copyerr
	call bcopy
	onfault_clr_and_load t0, t1

0:
	mv a0, zero
	ret
END(copyin)

/*
 * int copyout(const void *kaddr, void *udaddr, size_t len)
 *
 * Copy specified amount of data from kernel to the user space.
 */
ENTRY(copyout)
	/* len > 0 */
	beqz a2, 0f

	/* (uintptr_t)udaddr < (uintptr_t)(udaddr + len) */
	PTR_ADD t0, a1, a2
	bgeu a1, t0, reterr

	/* (uintptr_t)(udaddr + len) <= USER_SPACE_END */
	REG_LI t1, USER_SPACE_END
	bgtu t0, t1, reterr

	onfault_set_and_save t0, t1, copyerr
	call bcopy
	onfault_clr_and_load t0, t1

0:
	mv a0, zero
	ret
END(copyout)

ENTRY(copyerr)
	ctx_load
reterr:
	REG_LI a0, EFAULT
	ret
END(copyerr)

/*
 * bool try_load_word(unsigned *ptr, unsigned *val_p)
 */
ENTRY(try_load_word)
	onfault_set t0, t1, fault
	INT_L t2, (a0)
	onfault_clr t0, t1
	INT_S t2, (a1)
	REG_LI a0, 1
	ret
END(try_load_word)

/*
 * bool try_store_word(unsigned *ptr, unsigned val)
 */
ENTRY(try_store_word)
	onfault_set t0, t1, fault
	INT_S a1, (a0)
	onfault_clr t0, t1
	REG_LI a0, 1
	ret
END(try_store_word)

ENTRY(fault)
	mv a0, zero
	ret
END(fault)

# vim: sw=8 ts=8 et
