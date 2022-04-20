#include <sys/syscall.h>
#include <riscv/asm.h>

ENTRY(sigcode)
	mv a0, sp	/* address of ucontext to restore */
	li a7, SYS_sigreturn
	ecall

	/* Just in case `sigreturn` fails. */
	ebreak

/* `esigcode` is used just to compute size of the following code. */
EXPORT(esigcode)
END(sigcode)

# vim: sw=8 ts=8 et
