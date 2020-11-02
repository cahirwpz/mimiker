/*	$NetBSD: asm.h,v 1.30 2019/01/27 04:52:07 dholland Exp $	*/

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

#ifndef _AARCH64_ASM_H_
#define _AARCH64_ASM_H_

#include <aarch64/cdefs.h>

#define _C_LABEL(x) x

#define _SENTRY(x)                                                             \
  .align 2;                                                                    \
  .type x, @function;                                                          \
  x:
#define _ENTRY(x)                                                              \
  .globl x;                                                                    \
  _SENTRY(x)
#define _END(x) .size x, .- x

#ifdef GPROF
#define _PROF_PROLOGUE                                                         \
  mov x9, x30;                                                                 \
  bl __mcount
#else
#define _PROF_PROLOGUE
#endif

/* Global procedure start. */
#define ENTRY(y)                                                               \
  .text;                                                                       \
  _ENTRY(_C_LABEL(y));                                                         \
  .cfi_startproc;                                                              \
  _PROF_PROLOGUE
/* As above, without profiling. */
#define ENTRY_NP(y)                                                            \
  .text;                                                                       \
  _ENTRY(_C_LABEL(y));                                                         \
  .cfi_startproc
/* Local procedure start. */
#define SENTRY(y)                                                              \
  .text;                                                                       \
  _SENTRY(_C_LABEL(y));                                                        \
  .cfi_startproc;                                                              \
  _PROF_PROLOGUE
/* End of any procedure. */
#define END(y)                                                                 \
  .cfi_endproc;                                                                \
  _END(_C_LABEL(y))

#define fp x29
#define lr x30

/*
 * Add a speculation barrier after the 'eret'. Some aarch64 cpus speculatively
 * execute instructions after 'eret', and this potentiates side-channel attacks.
 */
#define ERET                                                                   \
  eret;                                                                        \
  dsb sy;                                                                      \
  isb

#endif /* !_AARCH64_ASM_H_ */
