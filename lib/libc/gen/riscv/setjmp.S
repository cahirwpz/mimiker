#include <riscv/abi.h>
#include <riscv/asm.h>

#include "riscv/assym.h"

/* 
 * int setjmp(jmp_buf env)
 */
ENTRY(setjmp)
	PTR_ADDI	sp, sp, -CALLFRAME_SIZ
	PTR_S	ra, CALLFRAME_RA(sp)
	REG_S	s0, CALLFRAME_S0(sp)
	mv	s0, a0

	/* 
	 * Save current signalmask at `ucontext::uc_sigmask`.
	 * `int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);`
	 * 
	 * If `set` is NULL, then the signal mask is unchanged (i.e., `how` is
	 * ignored), but the current value of the signal mask is nevertheless
	 * returned in `oldset` (if it is not NULL).
	 */
	mv	a1, zero
 	PTR_ADDI	a2, s0, UC_MASK	/* `&env->uc_sigmask` */
	call	_C_LABEL(sigprocmask)
	bnez	a0, botch

	/* 
	 * Save `stack_t` at `ucontext::uc_stack`.
	 * `int sigaltstack(const stack_t *ss, stack_t *old_ss);`
	 *
	 * By specifying `ss` as NULL, and `old_ss` as a non-NULL value, one can
	 * obtain the current settings for the alternate signal stack without
	 * changing them.
	 */
	PTR_ADDI	a1, s0, UC_STACK	/* `&env->uc_stack` */
	call	_C_LABEL(sigaltstack)
	bnez	a0, botch

	/* Extract onstack flag from obtained stack. */
	INT_L	t0, (UC_STACK + SS_FLAGS)(s0)
	li	t1, SS_ONSTACK
	and	t0, t0, t1
	beqz	t0, 1f

	/* Set stack flag in `env`. */
	INT_L	t0, UC_FLAGS(s0)
	or	t0, t0, t1
	INT_S	t0, UC_FLAGS(s0)

1:
	/* Restore jmpbuf and move to `_setjmp`. */
	mv	a0, s0
	REG_L	s0, CALLFRAME_S0(sp)
	PTR_L	ra, CALLFRAME_RA(sp)
	PTR_ADDI	sp, sp, CALLFRAME_SIZ

	j	_C_LABEL(_setjmp)

botch:
	call	_C_LABEL(abort)
END(setjmp)

# vim: sw=8 ts=8 et
