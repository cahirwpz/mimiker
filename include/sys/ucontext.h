/*	$NetBSD: ucontext.h,v 1.19 2018/02/27 23:09:02 uwe Exp $	*/

/*-
 * Copyright (c) 1999, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein, and by Jason R. Thorpe.
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

#ifndef _SYS_UCONTEXT_H_
#define _SYS_UCONTEXT_H_

#include <sys/sigtypes.h>
#include <machine/mcontext.h>

typedef struct __ucontext ucontext_t;

struct __ucontext {
  /* TODO(pj): remove */
  int uc_onstack;
  int uc_fpused;

  unsigned int uc_flags;  /* properties */
  ucontext_t *uc_link;    /* context to resume */
  sigset_t uc_sigmask;    /* signals blocked in this context */
  stack_t uc_stack;       /* the stack used by this context */
  mcontext_t uc_mcontext; /* machine state */
};

/* uc_flags */
#define _UC_SIGMASK 0x01 /* valid uc_sigmask */
#define _UC_STACK 0x02   /* valid uc_stack */
#define _UC_CPU 0x04     /* valid GPR context in uc_mcontext */
#define _UC_FPU 0x08     /* valid FPU context in uc_mcontext */

#endif /* !_SYS_UCONTEXT_H_ */
