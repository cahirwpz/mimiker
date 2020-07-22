/*	$NetBSD: mcontext.h,v 1.21 2018/10/12 01:28:58 ryo Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein and by Jason R. Thorpe of Wasabi Systems, Inc.
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

#ifndef _AARCH64_MCONTEXT_H_
#define _AARCH64_MCONTEXT_H_

#include <sys/types.h>

/*
 * General register state
 */
#define _NGREG 35 /* GR0-30, SP, PC, APSR, TPIDR */
typedef uint64_t __greg_t;
typedef __greg_t __gregset_t[_NGREG];

#define _REG_R0 0
#define _REG_R1 1
#define _REG_R2 2
#define _REG_R3 3
#define _REG_R4 4
#define _REG_R5 5
#define _REG_R6 6
#define _REG_R7 7
#define _REG_R8 8
#define _REG_R9 9
#define _REG_R10 10
#define _REG_R11 11
#define _REG_R12 12
#define _REG_R13 13
#define _REG_R14 14
#define _REG_R15 15
#define _REG_CPSR 16

#define _REG_X0 0
#define _REG_X1 1
#define _REG_X2 2
#define _REG_X3 3
#define _REG_X4 4
#define _REG_X5 5
#define _REG_X6 6
#define _REG_X7 7
#define _REG_X8 8
#define _REG_X9 9
#define _REG_X10 10
#define _REG_X11 11
#define _REG_X12 12
#define _REG_X13 13
#define _REG_X14 14
#define _REG_X15 15
#define _REG_X16 16
#define _REG_X17 17
#define _REG_X18 18
#define _REG_X19 19
#define _REG_X20 20
#define _REG_X21 21
#define _REG_X22 22
#define _REG_X23 23
#define _REG_X24 24
#define _REG_X25 25
#define _REG_X26 26
#define _REG_X27 27
#define _REG_X28 28
#define _REG_X29 29
#define _REG_X30 30
#define _REG_X31 31
#define _REG_ELR 32
#define _REG_SPSR 33
#define _REG_TPIDR 34

/* Convenience synonyms */
#define _REG_RV _REG_X0
#define _REG_FP _REG_X29
#define _REG_LR _REG_X30
#define _REG_SP _REG_X31
#define _REG_PC _REG_ELR

/*
 * Floating point register state
 */
#define _NFREG 32 /* Number of SIMD registers */

typedef struct {
  union __freg {
    uint8_t __b8[16];
    uint16_t __h16[8];
    uint32_t __s32[4];
    uint64_t __d64[2];
  } __qregs[_NFREG] __aligned(16);
  uint32_t __fpcr; /* FPCR */
  uint32_t __fpsr; /* FPSR */
} __fregset_t;

typedef struct {
  __gregset_t __gregs; /* General Purpose Register set */
  __fregset_t __fregs; /* FPU/SIMD Register File */
  __greg_t __spare[8]; /* future proof */
} mcontext_t;

/* Machine-dependent uc_flags */
#define _UC_TLSBASE 0x00080000 /* see <sys/ucontext.h> */

/* Machine-dependent uc_flags for arm */
#define _UC_ARM_VFP 0x00010000 /* FPU field is VFP */

/* used by signal delivery to indicate status of signal stack */
#define _UC_SETSTACK 0x00020000
#define _UC_CLRSTACK 0x00040000

#define _UC_MACHINE_SP(uc) ((uc)->uc_mcontext.__gregs[_REG_SP])
#define _UC_MACHINE_FP(uc) ((uc)->uc_mcontext.__gregs[_REG_FP])
#define _UC_MACHINE_PC(uc) ((uc)->uc_mcontext.__gregs[_REG_PC])
#define _UC_MACHINE_INTRV(uc) ((uc)->uc_mcontext.__gregs[_REG_RV])

#define _UC_MACHINE_SET_PC(uc, pc) _UC_MACHINE_PC(uc) = (pc)

#endif /* !_AARCH64_MCONTEXT_H_ */
