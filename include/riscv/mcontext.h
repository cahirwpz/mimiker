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

#define _NGREG 35 /* GR1-31, PC, SR, TVAL, CAUSE */
#define _NFREG 33 /* F0-31, FCSR */

union __fpreg {
  uint64_t u_u64;
  double u_d;
};

typedef uint32_t __greg_t;
typedef __greg_t __gregset_t[_NGREG];
typedef union __fpreg __fregset_t[_NFREG];

#define _REG_RA 0
#define _REG_SP 1
#define _REG_GP 2
#define _REG_TP 3
#define _REG_T0 4
#define _REG_T1 5
#define _REG_T2 6
#define _REG_S0 7
#define _REG_S1 8
#define _REG_A0 9
#define _REG_A1 10
#define _REG_A2 11
#define _REG_A3 12
#define _REG_A4 13
#define _REG_A5 14
#define _REG_A6 15
#define _REG_A7 16
#define _REG_S2 17
#define _REG_S3 18
#define _REG_S4 19
#define _REG_S5 20
#define _REG_S6 21
#define _REG_S7 22
#define _REG_S8 23
#define _REG_S9 24
#define _REG_S10 25
#define _REG_S11 26
#define _REG_T3 27
#define _REG_T4 28
#define _REG_T5 29
#define _REG_T6 30

#define _REG_PC 31
#define _REG_SR 32
#define _REG_TVAL 33
#define _REG_CAUSE 34

#define _REG_RV _REG_A0

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
#define _UC_TLSBASE 0x00080000  /* see <sys/ucontext.h> */

#define _UC_MACHINE_SP(uc) ((uc)->uc_mcontext.__gregs[_REG_SP])
#define _UC_MACHINE_FP(uc) ((uc)->uc_mcontext.__gregs[_REG_S0])
#define _UC_MACHINE_PC(uc) ((uc)->uc_mcontext.__gregs[_REG_PC])
#define _UC_MACHINE_INTRV(uc) ((uc)->uc_mcontext.__gregs[_REG_RV])

#define _UC_MACHINE_SET_PC(uc, pc) _UC_MACHINE_PC(uc) = (pc)

#endif /* !_RISCV_MCONTEXT_H_ */
