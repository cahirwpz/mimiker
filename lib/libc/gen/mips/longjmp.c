/*	$NetBSD: longjmp.c,v 1.6 2016/01/24 16:01:43 christos Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christian Limpach and Matt Thomas.
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

#include <sys/types.h>
#include <ucontext.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

void longjmp(jmp_buf env, int val) {
  struct sigcontext *sc = (void *)env;
  ucontext_t *sc_uc = &sc->sc_uc;
  ucontext_t uc;

  /* Ensure non-zero SP and sigcontext magic number is present */
  if (sc_uc->uc_mcontext.__gregs[_REG_SP] == 0)
    goto err;

  /* Ensure non-zero return value */
  if (val == 0)
    val = 1;

  /*
   * Set _UC_{SET,CLR}STACK according to SS_ONSTACK.
   *
   * Restore the signal mask with sigprocmask() instead of _UC_SIGMASK,
   * since libpthread may want to interpose on signal handling.
   */
  uc.uc_flags =
    _UC_CPU | ((sc_uc->uc_flags & _UC_STACK) ? _UC_SETSTACK : _UC_CLRSTACK);

  sigprocmask(SIG_SETMASK, &sc_uc->uc_sigmask, NULL);

  /* Clear uc_link */
  uc.uc_link = 0;

  /* Save return value in context */
  uc.uc_mcontext.__gregs[_REG_V0] = val;

  /* Copy saved registers */
  uc.uc_mcontext.__gregs[_REG_S0] = sc_uc->uc_mcontext.__gregs[_REG_S0];
  uc.uc_mcontext.__gregs[_REG_S1] = sc_uc->uc_mcontext.__gregs[_REG_S1];
  uc.uc_mcontext.__gregs[_REG_S2] = sc_uc->uc_mcontext.__gregs[_REG_S2];
  uc.uc_mcontext.__gregs[_REG_S3] = sc_uc->uc_mcontext.__gregs[_REG_S3];
  uc.uc_mcontext.__gregs[_REG_S4] = sc_uc->uc_mcontext.__gregs[_REG_S4];
  uc.uc_mcontext.__gregs[_REG_S5] = sc_uc->uc_mcontext.__gregs[_REG_S5];
  uc.uc_mcontext.__gregs[_REG_S6] = sc_uc->uc_mcontext.__gregs[_REG_S6];
  uc.uc_mcontext.__gregs[_REG_S7] = sc_uc->uc_mcontext.__gregs[_REG_S7];
  uc.uc_mcontext.__gregs[_REG_S8] = sc_uc->uc_mcontext.__gregs[_REG_S8];
#if !defined(__mips_abicalls)
  uc.uc_mcontext.__gregs[_REG_GP] = sc_uc->uc_mcontext.__gregs[_REG_GP];
#endif
  uc.uc_mcontext.__gregs[_REG_SP] = sc_uc->uc_mcontext.__gregs[_REG_SP];
  uc.uc_mcontext.__gregs[_REG_RA] = sc_uc->uc_mcontext.__gregs[_REG_RA];
  uc.uc_mcontext.__gregs[_REG_EPC] = sc_uc->uc_mcontext.__gregs[_REG_EPC];

  /* Copy FP state */
  if (sc_uc->uc_flags & _UC_FPU) {
    /* FP saved regs are $f20 .. $f31 */
    memcpy(&uc.uc_mcontext.__fpregs.__fp_r.__fp_regs[20],
           &sc_uc->uc_mcontext.__fpregs.__fp_r.__fp_regs[20],
           (32 - 20) * sizeof(float));
    uc.uc_mcontext.__fpregs.__fp_csr = sc_uc->uc_mcontext.__fpregs.__fp_csr;
    /* XXX sc_fp_control */
    uc.uc_flags |= _UC_FPU;
  }

  setcontext(&uc);
err:
  longjmperror();
  abort();
  /* NOTREACHED */
}
