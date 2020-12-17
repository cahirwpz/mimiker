/*	$NetBSD: _setjmp.S,v 1.24 2012/07/08 00:59:34 matt Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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

#include <machine/cdefs.h>
#include <mips/asm.h>

#include "mips/assym.h"
#include "mips/SYS.h"

/*
 * C library -- _setjmp, _longjmp
 *
 * _longjmp(a,v) will generate a "return(v)" from the last call to _setjmp(a) by
 * restoring registers from the stack.
 *
 * The previous signal state is NOT restored.
 */

	.set	noreorder

LEAF(_setjmp)
	REG_PROLOGUE
	REG_S	ra, _OFFSETOF_SC_PC(a0)		# sc_pc = return address
	REG_S	s0, _OFFSETOF_SC_REGS_S0(a0)
	REG_S	s1, _OFFSETOF_SC_REGS_S1(a0)
	REG_S	s2, _OFFSETOF_SC_REGS_S2(a0)
	REG_S	s3, _OFFSETOF_SC_REGS_S3(a0)
	REG_S	s4, _OFFSETOF_SC_REGS_S4(a0)
	REG_S	s5, _OFFSETOF_SC_REGS_S5(a0)
	REG_S	s6, _OFFSETOF_SC_REGS_S6(a0)
	REG_S	s7, _OFFSETOF_SC_REGS_S7(a0)
	REG_S	sp, _OFFSETOF_SC_REGS_SP(a0)
	REG_S	s8, _OFFSETOF_SC_REGS_S8(a0)
	/* In O32, FP registers F20 .. F23 are callee-saved. */
	cfc1	v0, $31				# too bad cant check if FP used
	FP_S	$f22, _OFFSETOF_SC_FPREGS_F22(a0)
	FP_S	$f20, _OFFSETOF_SC_FPREGS_F20(a0)
	FP_S	$f21, _OFFSETOF_SC_FPREGS_F21(a0)
	FP_S	$f23, _OFFSETOF_SC_FPREGS_F23(a0)
	INT_S	v0, _OFFSETOF_SC_FPREGS_FCSR(a0)
	REG_EPILOGUE

	j	ra
	 move	v0, zero
END(_setjmp)

LEAF(_longjmp)
	PIC_PROLOGUE(_longjmp)
	PTR_SUBU	sp, sp, CALLFRAME_SIZ
	SAVE_GP(CALLFRAME_GP)

	REG_PROLOGUE
	REG_L		ra, _OFFSETOF_SC_PC(a0)
	PTR_ADDU	sp, sp, CALLFRAME_SIZ	# does not matter, sanity
	REG_S		a1, _OFFSETOF_SC_REGS_V0(a0)	# save return value
	REG_L		s0, _OFFSETOF_SC_REGS_S0(a0)
	REG_L		s1, _OFFSETOF_SC_REGS_S1(a0)
	REG_L		s2, _OFFSETOF_SC_REGS_S2(a0)
	REG_L		s3, _OFFSETOF_SC_REGS_S3(a0)
	REG_L		s4, _OFFSETOF_SC_REGS_S4(a0)
	REG_L		s5, _OFFSETOF_SC_REGS_S5(a0)
	REG_L		s6, _OFFSETOF_SC_REGS_S6(a0)
	REG_L		s7, _OFFSETOF_SC_REGS_S7(a0)
	# get fpu status
	INT_L		v0, _OFFSETOF_SC_FPREGS_FCSR(a0)
	REG_L		sp, _OFFSETOF_SC_REGS_SP(a0)
	REG_L		s8, _OFFSETOF_SC_REGS_S8(a0)
	ctc1		v0, $31
	/* In O32, FP registers F20 .. F23 are callee-saved. */
	FP_L	$f22, _OFFSETOF_SC_FPREGS_F22(a0)
	FP_L	$f20, _OFFSETOF_SC_FPREGS_F20(a0)
	FP_L	$f21, _OFFSETOF_SC_FPREGS_F21(a0)
	FP_L	$f23, _OFFSETOF_SC_FPREGS_F23(a0)

	REG_EPILOGUE
	j	ra
	 move	v0, a1			# get return value in 1st arg
END(_longjmp)
