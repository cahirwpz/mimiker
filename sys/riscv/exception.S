#include <riscv/asm.h>

	.global user_exc_leave
	.global kern_exc_leave

ENTRY(cpu_exception_handler)
	# Not implemented!
	csrr a0, scause
	csrr a1, sepc
	csrr a2, stval
	j .
END(cpu_exception_handler)

ENTRY(user_exception_handler)
	# Not implemented!
user_exc_leave:
	j .
END(user_exception_handler)

ENTRY(kernel_exception_handler)
	# Not implemented!
kern_exc_leave:
	j .
END(kernel_exception_handler)

# vim: sw=8 ts=8 et
