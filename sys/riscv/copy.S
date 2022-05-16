#include <sys/errno.h>
#include <riscv/asm.h>
#include <riscv/riscvreg.h>

#include "assym.h"

/*
 * Enable permit Supervisor User Memory access (SUM).
 */
.macro enter_user_access tmp
#if SUM
	REG_LI \tmp, SSTATUS_SUM
	csrs sstatus, \tmp
#endif
.endm

/*
 * Disable permit Supervisor User Memory access (SUM).
 */
.macro exit_user_access tmp
#if SUM
	REG_LI \tmp, SSTATUS_SUM
	csrc sstatus, \tmp
#endif
.endm

.macro onfault_set tmp, td, func
	PTR_L	\td, PCPU_CURTHREAD(tp)
	PTR_LA	\tmp, \func
	PTR_S	\tmp, TD_ONFAULT(\td)
	enter_user_access \tmp
.endm

.macro onfault_clr tmp, td
	exit_user_access \tmp
	PTR_L	\td, PCPU_CURTHREAD(tp)
	PTR_S	zero, TD_ONFAULT(tp)
.endm

/*
 * copycommon - common copy routine
 *
 * a0 - Source address
 * a1 - Destination address
 * a2 - Size of copy
 *
 * Taken from `FreeBSD/sys/riscv/riscv/copyinout.S`.
 */
.macro copycommon
	REG_LI	t2, XLEN_BYTES
	blt	a2, t2, 4f		/* Byte-copy if len < XLEN_BYTES */

	/*
	 * Compare lower bits of src and dest.
	 * If they are aligned with each other, we can do word copy.
	 */
	andi	t0, a0, (XLEN_BYTES-1)	/* Low bits of src */
	andi	t1, a1, (XLEN_BYTES-1)	/* Low bits of dest */
	bne	t0, t1, 4f		/* Misaligned. Go to byte copy */
	beqz	t0, 2f			/* Already word-aligned, skip ahead */

	/* Byte copy until the first word-aligned address. */
1:	lb	a4, 0(a0)		/* Load byte from src */
	PTR_ADDI	a0, a0, 1
	sb	a4, 0(a1)		/* Store byte in dest */
	PTR_ADDI	a1, a1, 1
	LONG_ADDI	a2, a2, -1	/* len-- */
	andi	t0, a0, (XLEN_BYTES-1)
	bnez	t0, 1b
	j	3f

	/* Copy words. */
2:	LONG_L	a4, 0(a0)			/* Load word from src */
	PTR_ADDI	a0, a0, XLEN_BYTES
	LONG_S	a4, 0(a1)			/* Store word in dest */
	PTR_ADDI	a1, a1, XLEN_BYTES
	LONG_ADDI	a2, a2, -XLEN_BYTES	/* len -= XLEN_BYTES */
3:	bgeu	a2, t2, 2b			/* Again if len >= XLEN_BYTES */

	/* Check if we're finished. */
	beqz	a2, 5f

	/* Copy any remaining bytes. */
4:	lb	a4, 0(a0)		/* Load byte from src */
	PTR_ADDI	a0, a0, 1
	sb	a4, 0(a1)		/* Store byte in dest */
	PTR_ADDI	a1, a1, 1
	LONG_ADDI	a2, a2, -1	/* len-- */
	bnez	a2, 4b

5:
	.endm

/*
 * int copyin(const void *udaddr, void *kaddr, size_t len)
 *
 * Copy specified amount of data from user space into the kernel.
 */
ENTRY(copyin)
	/* len > 0 */
	beqz	a2, 1f

	/* (uintptr_t)udaddr < (uintptr_t)(udaddr + len) */
	PTR_ADD	t0, a0, a2
	bgeu	a0, t0, copyerr

	/* (uintptr_t)(udaddr + len) <= USER_SPACE_END */
	REG_LI	t1, USER_SPACE_END
	bgtu	t0, t1, copyerr

	onfault_set t0, t1, copyerr
	copycommon
	onfault_clr t0, t1

1:	mv	a0, zero
	ret
END(copyin)

/*
 * int copyout(const void *kaddr, void *udaddr, size_t len)
 *
 * Copy specified amount of data from kernel to the user space.
 */
ENTRY(copyout)
	/* len > 0 */
	beqz	a2, 1f

	/* (uintptr_t)udaddr < (uintptr_t)(udaddr + len) */
	PTR_ADD	t0, a1, a2
	bgeu	a1, t0, copyerr

	/* (uintptr_t)(udaddr + len) <= USER_SPACE_END */
	REG_LI	t1, USER_SPACE_END
	bgtu	t0, t1, copyerr

	onfault_set t0, t1, copyerr
	copycommon
	onfault_clr t0, t1

1:	mv	a0, zero
	ret
END(copyout)

/*
 * int copyinstr(const void *udaddr, void *kaddr, size_t len, size_t *lencopied)
 *
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 *
 * Taken from `FreeBSD/sys/riscv/riscv/copyinout.S`.
 */
ENTRY(copyinstr)
	mv	a5, zero		/* count = 0 */

	/* len > 0 */
	beqz	a2, 3f

	/* (uintptr_t)udaddr < (uintptr_t)(udaddr + len) */
	PTR_ADD	t0, a0, a2
	bgeu	a0, t0, copyerr

	/* (uintptr_t)(udaddr + len) <= USER_SPACE_END */
	REG_LI	t1, USER_SPACE_END
	bgtu	t0, t1, copyerr

	onfault_set t0, t1, copyerr

1:	lb	a4, 0(a0)		/* Load from uaddr */
	PTR_ADDI	a0, a0, 1
	sb	a4, 0(a1)		/* Store in kaddr */
	PTR_ADDI	a1, a1, 1
	beqz	a4, 2f
	LONG_ADDI	a2, a2, -1	/* len-- */
	LONG_ADDI	a5, a5, 1	/* count++ */
	bnez	a2, 1b

2:	onfault_clr t0, t1

3:	beqz	a3, 4f			/* Check if done != NULL */
	LONG_ADDI	a5, a5, 1	/* count++ */
	LONG_S	a5, 0(a3)		/* done = count */

4:	mv	a0, zero		/* return 0 */
	beqz	a4, 5f
	REG_LI	a0, ENAMETOOLONG

5:	ret
END(copyinstr)

ENTRY(copyerr)
	REG_LI	a0, EFAULT
	ret
END(copyerr)

/*
 * bool try_load_word(unsigned *ptr, unsigned *val_p)
 */
ENTRY(try_load_word)
	onfault_set t0, t1, fault
	INT_L	t2, (a0)
	onfault_clr t0, t1
	INT_S	t2, (a1)
	REG_LI	a0, 1
	ret
END(try_load_word)

/*
 * bool try_store_word(unsigned *ptr, unsigned val)
 */
ENTRY(try_store_word)
	onfault_set t0, t1, fault
	INT_S	a1, (a0)
	onfault_clr t0, t1
	REG_LI	a0, 1
	ret
END(try_store_word)

ENTRY(fault)
	mv	a0, zero
	ret
END(fault)

# vim: sw=8 ts=8 et
