/*	$NetBSD: profile.h,v 1.22 2020/07/26 08:08:41 simonb Exp $	*/

/*
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
 *	@(#)profile.h	8.1 (Berkeley) 6/10/93
 */

#define _MCOUNT_DECL                                                           \
  void __attribute__((no_instrument_function)) __cyg_profile_func_enter
void __cyg_profile_func_exit(void *this_fn, void *call_site)
  __attribute__((no_instrument_function));

#define MCOUNT                                                                 \
  __asm(".globl _cyg_profile_func_enter;"                                      \
        ".type _cyg_profile_func_enter,@function;"                             \
        "_cyg_profile_func_enter:;"                                            \
        ".set noreorder;"                                                      \
        ".set noat;"                                                           \
        "addu $29,$29,-16;"                                                    \
        "sw $4,8($29);"                                                        \
        "sw $5,12($29);"                                                       \
        "sw $6,16($29);"                                                       \
        "sw $7,20($29);"                                                       \
        "sw $1,0($29);"                                                        \
        "sw $31,4($29);"                                                       \
        "move $5,$31;"                                                         \
        "move $4,$1;"                                                          \
        "jal _cyg_profile_func_enter;"                                         \
        "nop;"                                                                 \
        "lw $4,8($29);"                                                        \
        "lw $5,12($29);"                                                       \
        "lw $6,16($29);"                                                       \
        "lw $7,20($29);"                                                       \
        "lw $31,4($29);"                                                       \
        "lw $1,0($29);"                                                        \
        "addu $29,$29,24;"                                                     \
        "j $31;"                                                               \
        "move $31,$1;"                                                         \
        ".set reorder;"                                                        \
        ".set at");

#define MCOUNT_ENTER cpu_intr_disable()

#define MCOUNT_EXIT cpu_intr_enable()
