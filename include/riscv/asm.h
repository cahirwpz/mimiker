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

#define _ENTRY(x)                                                              \
  .globl _C_LABEL(x);                                                          \
  .p2align 2;                                                                  \
  .type _C_LABEL(x), @function;                                                \
  _C_LABEL(x) :

#define _END(x) .size x, .- x

#define ENTRY(x)                                                               \
  .text;                                                                       \
  _ENTRY(_C_LABEL(x));                                                         \
  .cfi_startproc

#define END(x)                                                                 \
  .cfi_endproc;                                                                \
  _END(_C_LABEL(x))

#define EXPORT(x)                                                              \
  .globl _C_LABEL(x);                                                          \
  _C_LABEL(x) :

#define LOAD_GP()                                                              \
  .option push;                                                                \
  .option norelax;                                                             \
  PTR_LA gp, __global_pointer$;                                                \
  .option pop

#define SAVE_REG(reg, offset) REG_S reg, (offset)(sp)

#define SAVE_REG_CFI(reg, offset)                                              \
  SAVE_REG(reg, offset);                                                       \
  .cfi_rel_offset reg, offset

#define SAVE_CSR(csr, offset, tmp)                                             \
  csrr tmp, csr;                                                               \
  SAVE_REG(tmp, offset)

#define LOAD_REG(reg, offset) REG_L reg, (offset)(sp)

#define LOAD_CSR(csr, offset, tmp)                                             \
  LOAD_REG(tmp, offset);                                                       \
  csrw csr, tmp

#define ENABLE_SV_FPU(tmp)                                                     \
  li tmp, SSTATUS_FS_INITIAL;                                                  \
  csrs sstatus, tmp

#define DISABLE_SV_FPU(tmp)                                                    \
  li tmp, SSTATUS_FS_MASK;                                                     \
  csrc sstatus, tmp

#define SAVE_FPU_REG(reg, offset, dst) FP_S reg, (offset)(dst)

#define SAVE_FCSR(dst, tmp)                                                    \
  frcsr tmp;                                                                   \
  REG_S tmp, (FPU_CTX_FCSR)(dst)

#define LOAD_FPU_REG(reg, offset, src) FP_L reg, (offset)(src)

#define LOAD_FCSR(src, tmp)                                                    \
  REG_L tmp, (FPU_CTX_FCSR)(src);                                              \
  fscsr tmp

#ifdef __riscv_d
#define FP_L fld
#define FP_S fsd
#elif defined(__riscv_f)
#define FP_L flw
#define FP_S fsw
#endif

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
#if __riscv_xlen == 64
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
#if __riscv_xlen == 64
#define INT_ADD addw
#define INT_ADDI addiw
#define INT_SUB subw
#define INT_SUBI subiw
#define INT_SLLI slliw
#define INT_SLL sllw
#define INT_SRLI srliw
#define INT_SRL srlw
#define INT_SRAI sraiw
#define INT_SRA sraw
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
#if __riscv_xlen == 64
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
#define REG_SUB sub
#define REG_SLLI slli
#define REG_SLL sll
#define REG_SRLI srli
#define REG_SRL srl
#define REG_SRAI srai
#define REG_SRA sra
#if __riscv_xlen == 64
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
