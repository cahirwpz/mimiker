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

#ifndef _RISCV_ABI_H_
#define _RISCV_ABI_H_

#if __riscv_xlen == 64
#define SZREG 8
#else
#define SZREG 4
#endif

#define STACK_ALIGN 16

/*
 *  standard callframe {
 *  	register_t cf_sp;		frame pointer
 *  	register_t cf_ra;		return address
 *  };
 */
#define CALLFRAME_SIZ (SZREG * 4)
#define CALLFRAME_S1 (CALLFRAME_SIZ - 4 * SZREG)
#define CALLFRAME_S0 (CALLFRAME_SIZ - 3 * SZREG)
#define CALLFRAME_SP (CALLFRAME_SIZ - 2 * SZREG)
#define CALLFRAME_RA (CALLFRAME_SIZ - 1 * SZREG)

#define FUNC_MAXREGARGS 8

#endif /* !_RISCV_ABI_H_ */
