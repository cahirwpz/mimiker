/*	$NetBSD: sigsetjmp.S,v 1.9 2009/12/14 01:07:42 matt Exp $	*/

/*-
 * Copyright (c) 1991, 1993, 1995,
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Havard Eidnes.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/syscall.h>
#include <mips/asm.h>

#include "mips/assym.h"
#include "mips/SYS.h"

/*
 * C library -- sigsetjmp, siglongjmp
 *
 *	siglongjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	sigsetjmp(a, savemask)
 * by restoring registers from the stack,
 * and dependent on savemask restores the
 * signal mask.
 */

LEAF(sigsetjmp)
	PIC_PROLOGUE(sigsetjmp)
	bnez	a1, 1f			# do saving of signal mask?
	REG_S	a1, UC_FLAGS(a0)	# savemask is 0
	PIC_TAILCALL(_setjmp)		# doesn't save signal mask

1:	li	a1, _UC_SIGMASK
	REG_S	a1, UC_FLAGS(a0)	# valid user context sigmask
	PIC_TAILCALL(setjmp)		# saves signal mask
END(sigsetjmp)

LEAF(siglongjmp)
	PIC_PROLOGUE(siglongjmp)
	REG_L	t0, UC_FLAGS(a0)	# get "savemask"
	and	t0, t0, _UC_SIGMASK
	beq	t0, _UC_SIGMASK, 1f	# restore signal mask?
	PIC_TAILCALL(_longjmp)		# doesn't restore signal mask

1:	PIC_TAILCALL(longjmp)		# restores signal mask
END(siglongjmp)
