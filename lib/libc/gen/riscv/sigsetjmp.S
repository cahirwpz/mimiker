#include <riscv/asm.h>

ENTRY(sigsetjmp)
	# Not implemented!
	j .
END(sigsetjmp)

ENTRY(siglongjmp)
	# Not implemented!
	j .
END(siglongjmp)

# vim: sw=8 ts=8 et
