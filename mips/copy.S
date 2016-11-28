/* Mostly copied from FreeBSD mips/support.S */

#include <errno.h>
#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/pcpu.h>

#include "assym.h"

	.set	noreorder		# Noreorder is default style!

/*
 * int copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *lencopied)
 *
 * Copy a NIL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NIL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
LEAF(copystr)
	move		t0, a2
	beq		a2, zero, 4f
1:
	lbu		v0, 0(a0)
	PTR_SUBU	a2, a2, 1
	beq		v0, zero, 2f
	sb		v0, 0(a1)		# each byte until NIL
	PTR_ADDU	a0, a0, 1
	bne		a2, zero, 1b		# less than maxlen
	PTR_ADDU	a1, a1, 1
4:
	li		v0, ENAMETOOLONG	# run out of space
2:
	beq		a3, zero, 3f		# return num. of copied bytes
	PTR_SUBU	a2, t0, a2		# if the 4th arg was non-NULL
	PTR_S		a2, 0(a3)
3:
	j		ra			# v0 is 0 or ENAMETOOLONG
	nop
END(copystr)

/*
 * int copyinstr(const void *udaddr, void *kaddr, size_t len, size_t *lencopied)
 *
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 */
NESTED(copyinstr, CALLFRAME_SIZ, ra)
	PTR_SUBU sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, (CALLFRAME_RA - CALLFRAME_SIZ)
	LA	v0, copyerr
	blt	a0, zero, copyerr       # make sure address is in user space
	REG_S	ra, CALLFRAME_RA(sp)
	LOAD_PCPU(v1)
	jal	copystr
	PTR_S	v0, TD_ONFAULT(v1)
	REG_L	ra, CALLFRAME_RA(sp)
	LOAD_PCPU(v1)
	PTR_S	zero, TD_ONFAULT(v1)
	j	ra
	PTR_ADDU sp, sp, CALLFRAME_SIZ
END(copyinstr)

/*
 * int copyin(const void *udaddr, void *kaddr, size_t len)
 *
 * Copy specified amount of data from user space into the kernel.
 */
NESTED(copyin, CALLFRAME_SIZ, ra)
	PTR_SUBU sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, (CALLFRAME_RA - CALLFRAME_SIZ)
	LA	v0, copyerr
	blt	a0, zero, copyerr       # make sure address is in user space
	REG_S	ra, CALLFRAME_RA(sp)
	LOAD_PCPU(v1)
	jal	bcopy
	PTR_S	v0, TD_ONFAULT(v1)
	REG_L	ra, CALLFRAME_RA(sp)
	LOAD_PCPU(v1)
	PTR_S	zero, TD_ONFAULT(v1)
	PTR_ADDU sp, sp, CALLFRAME_SIZ
	j	ra
	move	v0, zero
END(copyin)

/*
 * int copyout(const void *kaddr, void *udaddr, size_t len)
 *
 * Copy specified amount of data from kernel to the user space.
 */
NESTED(copyout, CALLFRAME_SIZ, ra)
	PTR_SUBU	sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, (CALLFRAME_RA - CALLFRAME_SIZ)
	LA	v0, copyerr
	blt	a1, zero, copyerr       # make sure address is in user space
	REG_S	ra, CALLFRAME_RA(sp)
	LOAD_PCPU(v1)
	PTR_L	v1, PC_CURPCB(v1)
	jal	bcopy
	PTR_S	v0, TD_ONFAULT(v1)
	REG_L	ra, CALLFRAME_RA(sp)
	LOAD_PCPU(v1)
	PTR_S	zero, TD_ONFAULT(v1)
	PTR_ADDU sp, sp, CALLFRAME_SIZ
	j	ra
	move	v0, zero
END(copyout)

LEAF(copyerr)
	REG_L	ra, CALLFRAME_RA(sp)
	PTR_ADDU	sp, sp, CALLFRAME_SIZ
	j	ra
	li	v0, EFAULT			# return error
END(copyerr)

/*
 * int32_t fuword32(const void *ptr)
 * int suword32(void *ptr, int32_t word)
 * 
 * {fu,su}{iword32} fetch / store 32-bit word from / to the user space.
 */

LEAF(fuword32)
	LA	v0, fswberr
	blt	a0, zero, fswberr	# make sure address is in user space
	nop
	LOAD_PCPU(v1)
	PTR_S	v0, TD_ONFAULT(v1)
	lw	v0, 0(a0)		# fetch word
	j	ra
	PTR_S	zero, TD_ONFAULT(v1)
END(fuword32)

LEAF(suword32)
	LA	v0, fswberr
	blt	a0, zero, fswberr	# make sure address is in user space
	nop
	LOAD_PCPU(v1)
	PTR_S	v0, TD_ONFAULT(v1)
	sw	a1, 0(a0)		# store short
	PTR_S	zero, TD_ONFAULT(v1)
	j	ra
	move	v0, zero
END(suword32)

LEAF(fswberr)
	j	ra
	li	v0, -1
END(fswberr)

# vim: sw=8 ts=8 et
