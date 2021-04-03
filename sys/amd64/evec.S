#include <amd64/asm.h>

ENTRY(handle_kern_trap)
	# not implemented
	mov 0, %rax
END(handle_kern_trap)

ENTRY(kern_exc_leave)
	# not implemented
	mov 0, %rax
END(kern_exc_leave)

ENTRY(handle_kern_intr)
	# not implemented
	mov 0, %rax
END(handle_kern_intr)

ENTRY(handle_user_trap)
	# not implemented
	mov 0, %rax
END(handle_user_trap)

ENTRY(user_exc_leave)
	# not implemented
	mov 0, %rax
END(user_exc_leave)

ENTRY(handle_user_intr)
	# not implemented
	mov 0, %rax
END(handle_user_intr)

# vim: sw=8 ts=8 et
