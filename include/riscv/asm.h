/*	$NetBSD: asm.h,v 1.6 2021/05/01 07:05:07 skrll Exp $	*/

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

#ifndef _RISCV_ASM_H
#define _RISCV_ASM_H

#define _C_LABEL(x) x

#define __CONCAT(x, y) x##y
#define __STRING(x) #x

#define ___CONCAT(x, y) __CONCAT(x, y)

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

#define _ENTRY(x)                                                              \
  .globl _C_LABEL(x);                                                          \
  .align 4;                                                                    \
  .type _C_LABEL(x), @function;                                                \
  _C_LABEL(x) :

#define ENTRY(x)                                                               \
  .text;                                                                       \
  _ENTRY(x)
#define END(x) .size _C_LABEL(x), .- _C_LABEL(x)

#define FP_L fld
#define FP_S fsd

/*
 * These macros hide the use of rv32 and rv64 instructions from the
 * assembler to prevent the assembler from generating 64-bit style
 * ABI calls.
 */
#define PTR_ADD add
#define PTR_ADDI addi
#define PTR_SUB sub
#define PTR_SUBI subi
#define PTR_LA la
#define PTR_SLLI slli
#define PTR_SLL sll
#define PTR_SRLI srli
#define PTR_SRL srl
#define PTR_SRAI srai
#define PTR_SRA sra
#if _LP64
#define PTR_L ld
#define PTR_S sd
#define PTR_LR lr.d
#define PTR_SC sc.d
#define PTR_WORD .dword
#define PTR_SCALESHIFT 3
#else
#define PTR_L lw
#define PTR_S sw
#define PTR_LR lr.w
#define PTR_SC sc.w
#define PTR_WORD .word
#define PTR_SCALESHIFT 2
#endif

#define INT_L lw
#define INT_LA la
#define INT_S sw
#define INT_LR lr.w
#define INT_SC sc.w
#define INT_WORD .word
#define INT_SCALESHIFT 2
#ifdef _LP64
#define INT_ADD addw
#define INT_ADDI addwi
#define INT_SUB subw
#define INT_SUBI subwi
#define INT_SLL sllwi
#define INT_SLLV sllw
#define INT_SRL srlwi
#define INT_SRLV srlw
#define INT_SRA srawi
#define INT_SRAV sraw
#else
#define INT_ADD add
#define INT_ADDI addi
#define INT_SUB sub
#define INT_SUBI subi
#define INT_SLLI slli
#define INT_SLL sll
#define INT_SRLI srli
#define INT_SRL srl
#define INT_SRAI srai
#define INT_SRA sra
#endif

#define LONG_LA la
#define LONG_ADD add
#define LONG_ADDI addi
#define LONG_SUB sub
#define LONG_SUBI subi
#define LONG_SLLI slli
#define LONG_SLL sll
#define LONG_SRLI srli
#define LONG_SRL srl
#define LONG_SRAI srai
#define LONG_SRA sra
#ifdef _LP64
#define LONG_L ld
#define LONG_S sd
#define LONG_LR lr.d
#define LONG_SC sc.d
#define LONG_WORD .quad
#define LONG_SCALESHIFT 3
#else
#define LONG_L lw
#define LONG_S sw
#define LONG_LR lr.w
#define LONG_SC sc.w
#define LONG_WORD .word
#define LONG_SCALESHIFT 2
#endif

#define REG_LI li
#define REG_ADD add
#define REG_SLLI slli
#define REG_SLL sll
#define REG_SRLI srli
#define REG_SRL srl
#define REG_SRAI srai
#define REG_SRA sra
#if _LP64
#define REG_L ld
#define REG_S sd
#define REG_LR lr.d
#define REG_SC sc.d
#define REG_SCALESHIFT 3
#else
#define REG_L lw
#define REG_S sw
#define REG_LR lr.w
#define REG_SC sc.w
#define REG_SCALESHIFT 2
#endif

#endif /* _RISCV_ASM_H */
