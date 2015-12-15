/*******************************************************************************
 *
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/******************************************************************************
*              file : $RCSfile: setjmp.S,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* This is a simple version of setjmp and longjmp for MIPS 32 and 64.

   Ian Lance Taylor, Cygnus Support, 13 May 1993.  */

#ifdef __mips16
/* This file contains 32 bit assembly code.  */
	.set nomips16
#endif

#define GPR_LAYOUT		\
	GPR_OFFSET ($16, 0);	\
	GPR_OFFSET ($17, 1);	\
	GPR_OFFSET ($18, 2);	\
	GPR_OFFSET ($19, 3);	\
	GPR_OFFSET ($20, 4);	\
	GPR_OFFSET ($21, 5);	\
	GPR_OFFSET ($22, 6);	\
	GPR_OFFSET ($23, 7);	\
	GPR_OFFSET ($29, 8);	\
	GPR_OFFSET ($30, 9);	\
	GPR_OFFSET ($31, 10)

#define NUM_GPRS_SAVED 11

#ifdef __mips_hard_float
#if _MIPS_SIM == _ABIN32
#define FPR_LAYOUT		\
	FPR_OFFSET ($f20, 0);	\
	FPR_OFFSET ($f22, 1);	\
	FPR_OFFSET ($f24, 2);	\
	FPR_OFFSET ($f26, 3);	\
	FPR_OFFSET ($f28, 4);	\
	FPR_OFFSET ($f30, 5);
#elif _MIPS_SIM == _ABI64
#define FPR_LAYOUT		\
	FPR_OFFSET ($f24, 0);	\
	FPR_OFFSET ($f25, 1);	\
	FPR_OFFSET ($f26, 2);	\
	FPR_OFFSET ($f27, 3);	\
	FPR_OFFSET ($f28, 4);	\
	FPR_OFFSET ($f29, 5);	\
	FPR_OFFSET ($f30, 6);	\
	FPR_OFFSET ($f31, 7);
/* This deals with the O32 FPXX and FP64 cases.  Here we must use
   sdc1 and ldc1 to access the FPRs.  These instruction require
   8-byte aligned addresses.  If the jump buffer is 8-byte aligned 
   we can not store $31 before the FPRs, as this will cause the buffer
   to become only 4-byte aligned.  Instead, the FPRs are placed first
   and $31 is put at the end.  */
#elif __mips_fpr == 0 || __mips_fpr == 64
#define FPR_LAYOUT		\
	and $8, $4, 4;	 	\
	bne $8, $0, 1f;		\
	GPR_OFFSET ($31, 22);	\
	addiu $4, $4, -4;	\
1:				\
	FPR_OFFSET ($f20, 0);  	\
	FPR_OFFSET ($f22, 2);	\
	FPR_OFFSET ($f24, 4);	\
	FPR_OFFSET ($f26, 6);	\
	FPR_OFFSET ($f28, 8);	\
	FPR_OFFSET ($f30, 10);
#else /* Assuming _MIPS_SIM == _ABIO32 */
#define FPR_LAYOUT		\
	FPR_OFFSET ($f20, 0);	\
	FPR_OFFSET ($f21, 1);	\
	FPR_OFFSET ($f22, 2);	\
	FPR_OFFSET ($f23, 3);	\
	FPR_OFFSET ($f24, 4);	\
	FPR_OFFSET ($f25, 5);	\
	FPR_OFFSET ($f26, 6);	\
	FPR_OFFSET ($f27, 7);	\
	FPR_OFFSET ($f28, 8);	\
	FPR_OFFSET ($f29, 9);	\
	FPR_OFFSET ($f30, 10);	\
	FPR_OFFSET ($f31, 11);
#endif
#else
#define FPR_LAYOUT
#endif
	
#ifdef __mips64
#define BYTES_PER_WORD 8
#define LOAD_GPR ld
#define LOAD_FPR ldc1
#define STORE_GPR sd
#define STORE_FPR sdc1
#else
#define LOAD_GPR lw
#define STORE_GPR sw
#define BYTES_PER_WORD 4
#if __mips_fpr == 0 || __mips_fpr == 64
#define LOAD_FPR ldc1
#define STORE_FPR sdc1
#else
#define LOAD_FPR lwc1
#define STORE_FPR swc1
#endif
#endif

#define GPOFF(INDEX) (INDEX * BYTES_PER_WORD)
#define FPOFF(INDEX) ((INDEX + NUM_GPRS_SAVED) * BYTES_PER_WORD)

/* int setjmp (jmp_buf);  */
	.globl	setjmp
	.ent	setjmp
setjmp:
	.frame	$sp,0,$31

#define GPR_OFFSET(REG, INDEX) STORE_GPR REG,GPOFF(INDEX)($4)
#define FPR_OFFSET(REG, INDEX) STORE_FPR REG,FPOFF(INDEX)($4)
	GPR_LAYOUT
	FPR_LAYOUT
#undef GPR_OFFSET
#undef FPR_OFFSET

	move	$2,$0
	j	$31

	.end	setjmp

/* volatile void longjmp (jmp_buf, int);  */
	.globl	longjmp
	.ent	longjmp
longjmp:
	.frame	$sp,0,$31

#define GPR_OFFSET(REG, INDEX) LOAD_GPR REG,GPOFF(INDEX)($4)
#define FPR_OFFSET(REG, INDEX) LOAD_FPR REG,FPOFF(INDEX)($4)
	GPR_LAYOUT
	FPR_LAYOUT
#undef GPR_OFFSET
#undef FPR_OFFSET

	bne	$5,$0,1f
	li	$5,1
1:
	move	$2,$5
	j	$31

	.end longjmp
