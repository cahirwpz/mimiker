/* $NetBSD: mcontext.h,v 1.6 2020/03/14 16:12:16 skrll Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* MODIFIED */

#ifndef _RISCV_MCONTEXT_H_
#define _RISCV_MCONTEXT_H_

#include <sys/types.h>

#define _NGREG 32 /* GR1-31, PC */
#define _NFREG 33 /* F0-31, FCSR */

union __fpreg {
  uint64_t u_u64;
  double u_d;
};

typedef uint32_t __greg_t;
typedef __greg_t __gregset_t[_NGREG];
typedef union __fpreg __fregset_t[_NFREG];

#define _REG_X1 0
#define _REG_X2 1
#define _REG_X3 2
#define _REG_X4 3
#define _REG_X5 4
#define _REG_X6 5
#define _REG_X7 6
#define _REG_X8 7
#define _REG_X9 8
#define _REG_X10 9
#define _REG_X11 10
#define _REG_X12 11
#define _REG_X13 12
#define _REG_X14 13
#define _REG_X15 14
#define _REG_X16 15
#define _REG_X17 16
#define _REG_X18 17
#define _REG_X19 18
#define _REG_X20 19
#define _REG_X21 20
#define _REG_X22 21
#define _REG_X23 22
#define _REG_X24 23
#define _REG_X25 24
#define _REG_X26 25
#define _REG_X27 26
#define _REG_X28 27
#define _REG_X29 28
#define _REG_X30 29
#define _REG_X31 30
#define _REG_PC 31

#define _REG_RA _REG_X1
#define _REG_SP _REG_X2
#define _REG_GP _REG_X3
#define _REG_TP _REG_X4
#define _REG_S0 _REG_X8
#define _REG_RV _REG_X10
#define _REG_A0 _REG_X10

#define _REG_F0 0
#define _REG_FPCSR 32

typedef struct mcontext {
  __gregset_t __gregs; /* General Purpose Register set */
  __fregset_t __fregs; /* Floating Point Register set */
  __greg_t __spare[8]; /* future proof */
} mcontext_t;

#if defined(_MACHDEP) && defined(_KERNEL)

typedef struct ctx {
  __gregset_t __gregs; /* General Purpose Register set */
} ctx_t;

#define _REG(ctx, n) ((ctx)->__gregs[_REG_##n])

#endif /* !_MACHDEP && !_KERNEL */

/* Machine-dependent uc_flags */
#define _UC_SETSTACK 0x00010000 /* see <sys/ucontext.h> */
#define _UC_CLRSTACK 0x00020000 /* see <sys/ucontext.h> */

#endif /* !_RISCV_MCONTEXT_H_ */
