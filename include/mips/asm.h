/*	$NetBSD: asm.h,v 1.29 2000/12/14 21:29:51 jeffs Exp $	*/

/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1992, 1993
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
 *
 *	@(#)machAsmDefs.h	8.1 (Berkeley) 6/10/93
 *	JNPR: asm.h,v 1.10 2007/08/09 11:23:32 katta
 * $FreeBSD$
 */

/*
 * machAsmDefs.h --
 *
 *	Macros used when writing assembler programs.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsmDefs.h,
 *	v 1.2 89/08/15 18:28:24 rab Exp  SPRITE (DECWRL)
 */

#ifndef _MIPS_ASM_H_
#define _MIPS_ASM_H_

#include <machine/abi.h>
#include <machine/regdef.h>
#include <sys/endian.h>

#define _C_LABEL(x) x

#define AENT(x)                                                                \
  .globl _C_LABEL(x);                                                          \
  .aent _C_LABEL(x);                                                           \
  _C_LABEL(x) :

/*
 * WARN_REFERENCES: create a warning if the specified symbol is referenced
 */
#define WARN_REFERENCES(_sym, _msg)                                            \
  .section.gnu.warning.##_sym;                                                 \
  .ascii _msg;                                                                 \
  .text

/*
 * WEAK_ALIAS: create a weak alias.
 */
#define WEAK_ALIAS(alias, sym)                                                 \
  .weak alias;                                                                 \
  alias = sym

/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias, sym)                                               \
  .globl alias;                                                                \
  alias = sym

#define GLOBAL(sym)                                                            \
  .globl sym;                                                                  \
  sym:

#define ENTRY(sym)                                                             \
  .text;                                                                       \
  .globl sym;                                                                  \
  .ent sym;                                                                    \
  sym:

#define ASM_ENTRY(sym)                                                         \
  .text;                                                                       \
  .globl sym;                                                                  \
  .type sym, @function;                                                        \
  sym:

/*
 * LEAF
 *	A leaf routine does
 *	- call no other function,
 *	- never use any register that callee-saved (S0-S8), and
 *	- not use any local stack storage.
 */
#define LEAF(x)                                                                \
  .globl _C_LABEL(x);                                                          \
  .ent _C_LABEL(x), 0;                                                         \
  _C_LABEL(x) :;                                                               \
  .frame sp, 0, ra;                                                            \
  .cfi_startproc

/* Static/local leaf function. */
#define SLEAF(x)                                                               \
  .ent _C_LABEL(x), 0;                                                         \
  _C_LABEL(x) :;                                                               \
  .frame sp, 0, ra;                                                            \
  .cfi_startproc

/*
 * LEAF_NOPROFILE
 *	No profilable leaf routine.
 */
#define LEAF_NOPROFILE(x)                                                      \
  .globl _C_LABEL(x);                                                          \
  .ent _C_LABEL(x), 0;                                                         \
  _C_LABEL(x) :;                                                               \
  .frame sp, 0, ra;                                                            \
  .cfi_startproc

/*
 * XLEAF
 *	declare alternate entry to leaf routine
 */
#define XLEAF(x)                                                               \
  .globl _C_LABEL(x);                                                          \
  AENT(_C_LABEL(x));                                                           \
  _C_LABEL(x) :

/*
 * NESTED
 *	A function calls other functions and needs
 *	therefore stack space to save/restore registers.
 */
#define NESTED(x, fsize, retpc)                                                \
  .globl _C_LABEL(x);                                                          \
  .ent _C_LABEL(x), 0;                                                         \
  _C_LABEL(x) :;                                                               \
  .frame sp, fsize, retpc;                                                     \
  .cfi_startproc

#define SNESTED(x, fsize, retpc)                                               \
  .ent _C_LABEL(x), 0;                                                         \
  _C_LABEL(x) :;                                                               \
  .frame sp, fsize, retpc;                                                     \
  .cfi_startproc

#define NON_LEAF(x, fsize, retpc) NESTED(x, fsize, retpc)

/*
 * NESTED_NOPROFILE(x)
 *	No profilable nested routine.
 */
#define NESTED_NOPROFILE(x, fsize, retpc)                                      \
  .globl _C_LABEL(x);                                                          \
  .ent _C_LABEL(x), 0;                                                         \
  _C_LABEL(x) :;                                                               \
  .frame sp, fsize, retpc;                                                     \
  .cfi_startproc

/*
 * XNESTED
 *	declare alternate entry point to nested routine.
 */
#define XNESTED(x)                                                             \
  .globl _C_LABEL(x);                                                          \
  AENT(_C_LABEL(x));                                                           \
  _C_LABEL(x) :

/*
 * END
 *	Mark end of a procedure.
 */
#define END(x)                                                                 \
  .size _C_LABEL(x), .- _C_LABEL(x);                                           \
  .end _C_LABEL(x);                                                            \
  .cfi_endproc

/*
 * IMPORT -- import external symbol
 */
#define IMPORT(sym, size) .extern _C_LABEL(sym), size

/*
 * EXPORT -- export definition of symbol
 */
#define EXPORT(x)                                                              \
  .globl _C_LABEL(x);                                                          \
  _C_LABEL(x) :

/*
 * VECTOR
 *	exception vector entrypoint
 *	XXX: regmask should be used to generate .mask
 */
#define VECTOR(x, regmask)                                                     \
  .ent _C_LABEL(x), 0;                                                         \
  EXPORT(x);

#define VECTOR_END(x)                                                          \
  EXPORT(x##End);                                                              \
  END(x)

/*
 * Macros to panic and printf from assembly language.
 */
#define PANIC(msg)                                                             \
  PTR_LA a0, 9f;                                                               \
  jal _C_LABEL(panic);                                                         \
  nop;                                                                         \
  MSG(msg)

#define PANIC_KSEG0(msg, reg) PANIC(msg)

#define PRINTF(msg)                                                            \
  PTR_LA a0, 9f;                                                               \
  jal _C_LABEL(printf);                                                        \
  nop;                                                                         \
  MSG(msg)

#define MSG(msg)                                                               \
  .rdata;                                                                      \
  9 :.asciiz msg;                                                              \
  .text

#define ASMSTR(str)                                                            \
  .asciiz str;                                                                 \
  .align 3

#define ALSK 7    /* stack alignment */
#define ALMASK -7 /* stack alignment */
#define SZFPREG 4
#define FP_L lwc1
#define FP_S swc1

/*
 *   Endian-independent assembly-code aliases for unaligned memory accesses.
 */
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define LWHI lwr
#define LWLO lwl
#define SWHI swr
#define SWLO swl
#define REG_LHI lwr
#define REG_LLO lwl
#define REG_SHI swr
#define REG_SLO swl
#endif

#if _BYTE_ORDER == _BIG_ENDIAN
#define LWHI lwl
#define LWLO lwr
#define SWHI swl
#define SWLO swr
#define REG_LHI lwl
#define REG_LLO lwr
#define REG_SHI swl
#define REG_SLO swr
#endif

/*
 * While it would be nice to be compatible with the SGI
 * REG_L and REG_S macros, because they do not take parameters, it
 * is impossible to use them with the _MIPS_SIM_ABIX32 model.
 *
 * These macros hide the use of mips3 instructions from the
 * assembler to prevent the assembler from generating 64-bit style
 * ABI calls.
 */
#define PTR_ADD add
#define PTR_ADDI addi
#define PTR_ADDU addu
#define PTR_ADDIU addiu
#define PTR_SUB add
#define PTR_SUBI subi
#define PTR_SUBU subu
#define PTR_SUBIU subu
#define PTR_L lw
#define PTR_LA la
#define PTR_LI li
#define PTR_S sw
#define PTR_SLL sll
#define PTR_SLLV sllv
#define PTR_SRL srl
#define PTR_SRLV srlv
#define PTR_SRA sra
#define PTR_SRAV srav
#define PTR_LL ll
#define PTR_SC sc
#define PTR_WORD .word
#define PTR_SCALESHIFT 2

#define INT_ADD add
#define INT_ADDI addi
#define INT_ADDU addu
#define INT_ADDIU addiu
#define INT_SUB add
#define INT_SUBI subi
#define INT_SUBU subu
#define INT_SUBIU subu
#define INT_L lw
#define INT_LA la
#define INT_S sw
#define INT_SLL sll
#define INT_SLLV sllv
#define INT_SRL srl
#define INT_SRLV srlv
#define INT_SRA sra
#define INT_SRAV srav
#define INT_LL ll
#define INT_SC sc
#define INT_WORD .word
#define INT_SCALESHIFT 2

#define LONG_ADD add
#define LONG_ADDI addi
#define LONG_ADDU addu
#define LONG_ADDIU addiu
#define LONG_SUB add
#define LONG_SUBI subi
#define LONG_SUBU subu
#define LONG_SUBIU subu
#define LONG_L lw
#define LONG_LA la
#define LONG_S sw
#define LONG_SLL sll
#define LONG_SLLV sllv
#define LONG_SRL srl
#define LONG_SRLV srlv
#define LONG_SRA sra
#define LONG_SRAV srav
#define LONG_LL ll
#define LONG_SC sc
#define LONG_WORD .word
#define LONG_SCALESHIFT 2

#define REG_L lw
#define REG_S sw
#define REG_LI li
#define REG_ADDU addu
#define REG_SLL sll
#define REG_SLLV sllv
#define REG_SRL srl
#define REG_SRLV srlv
#define REG_SRA sra
#define REG_SRAV srav
#define REG_LL ll
#define REG_SC sc
#define REG_SCALESHIFT 2

#define MFC0 mfc0
#define MTC0 mtc0

#define CPRESTORE(r) .cprestore r
#define CPLOAD(r) .cpload r

#define SETUP_GP                                                               \
  .set push;                                                                   \
  .set noreorder;                                                              \
  .cpload t9;                                                                  \
  .set pop
#define SETUP_GPX(r)                                                           \
  .set push;                                                                   \
  .set noreorder;                                                              \
  move r, ra; /* save old ra */                                                \
  bal 7f;                                                                      \
  nop;                                                                         \
  7 :.cpload ra;                                                               \
  move ra, r;                                                                  \
  .set pop
#define SETUP_GPX_L(r, lbl)                                                    \
  .set push;                                                                   \
  .set noreorder;                                                              \
  move r, ra; /* save old ra */                                                \
  bal lbl;                                                                     \
  nop;                                                                         \
  lbl:                                                                         \
  .cpload ra;                                                                  \
  move ra, r;                                                                  \
  .set pop
#define SAVE_GP(x) .cprestore x

#define REG_PROLOGUE .set push
#define REG_EPILOGUE .set pop

#ifdef _KERNEL
#define GET_CPU_PCPU(reg) PTR_L reg, _C_LABEL(pcpup);
#endif /* !_KERNEL */

#endif /* !_MIPS_ASM_H_ */
