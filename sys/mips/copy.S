/* Mostly copied from FreeBSD mips/support.S */

#include <sys/errno.h>
#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/pcpu.h>

#include "assym.h"

# WARNING: copyin and copyout make unsafe assumption that memcpy routine does
# not modify caller saved registers nor stack pointer. If you're about to
# change memcpy implementation, make sure it does not violate the assumption!

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
	sb		v0, 0(a1)	        # each byte until NIL
	PTR_ADDU	a0, a0, 1
	bne		a2, zero, 1b	        # less than maxlen
	PTR_ADDU	a1, a1, 1
4:
	li		v0, ENAMETOOLONG        # run out of space
2:
	beq		a3, zero, 3f	        # return num. of copied bytes
	PTR_SUBU	a2, t0, a2	        # if the 4th arg was non-NULL
	PTR_S		a2, 0(a3)
3:
	j		ra		        # v0 is 0 or ENAMETOOLONG
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
	REG_S	ra, CALLFRAME_RA(sp)
	LA	v0, copyerr
	blt	a0, zero, copyerr       # make sure address is in user space
        nop
	LOAD_PCPU(v1)
        PTR_L   v1, PCPU_CURTHREAD(v1)
	jal	copystr
	PTR_S	v0, TD_ONFAULT(v1)
	REG_L	ra, CALLFRAME_RA(sp)
	LOAD_PCPU(v1)
        PTR_L   v1, PCPU_CURTHREAD(v1)
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
	REG_S	ra, CALLFRAME_RA(sp)
        .cfi_rel_offset ra, CALLFRAME_RA
	LA	v0, copyerr
	blt	a0, zero, copyerr       # make sure address is in user space
        nop
	LOAD_PCPU(v1)
        PTR_L   v1, PCPU_CURTHREAD(v1)
	jal	bcopy                   # bcopy uses a0-a2, v0-v1, t0-t3
	PTR_S	v0, TD_ONFAULT(v1)
	REG_L	ra, CALLFRAME_RA(sp)
        .cfi_restore ra
	LOAD_PCPU(v1)
        PTR_L   v1, PCPU_CURTHREAD(v1)
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
	PTR_SUBU sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, (CALLFRAME_RA - CALLFRAME_SIZ)
	REG_S	ra, CALLFRAME_RA(sp)
        .cfi_rel_offset ra, CALLFRAME_RA
	LA	v0, copyerr
	blt	a1, zero, copyerr       # make sure address is in user space
        nop
	LOAD_PCPU(v1)
        PTR_L   v1, PCPU_CURTHREAD(v1)
	jal	bcopy                   # bcopy uses a0-a2, v0-v1, t0-t3
	PTR_S	v0, TD_ONFAULT(v1)
	REG_L	ra, CALLFRAME_RA(sp)
        .cfi_restore ra
	LOAD_PCPU(v1)
        PTR_L   v1, PCPU_CURTHREAD(v1)
	PTR_S	zero, TD_ONFAULT(v1)
	PTR_ADDU sp, sp, CALLFRAME_SIZ
	j	ra
	move	v0, zero
END(copyout)

SLEAF(copyerr)
	REG_L	ra, CALLFRAME_RA(sp)
	PTR_ADDU sp, sp, CALLFRAME_SIZ
	j	ra
	li	v0, EFAULT		# return error
END(copyerr)

/* bool try_load_word(unsigned *ptr, unsigned *val_p) */
LEAF(try_load_word)
        LOAD_PCPU(v1)
        PTR_L   v1, PCPU_CURTHREAD(v1)
        LA      v0, fault
        PTR_S   v0, TD_ONFAULT(v1)
        lw      v0, 0(a0)
        PTR_S   zero, TD_ONFAULT(v1)
        sw      v0, 0(a1)
        j       ra
        li      v0, 1
END(try_load_word)

/* bool try_store_word(unsigned *ptr, unsigned val) */
LEAF(try_store_word)
        LA      v0, fault
        LOAD_PCPU(v1)
        PTR_L   v1, PCPU_CURTHREAD(v1)
        PTR_S   v0, TD_ONFAULT(v1)
        sw      a1, 0(a0)
        PTR_S   zero, TD_ONFAULT(v1)
        j       ra
        li      v0, 1
END(try_store_word)

SLEAF(fault)
        j       ra
        move    v0, zero
END(fault)

# vim: sw=8 ts=8 et

# vim: sw=8 ts=8 et
