/*-
 * Copyright (c) 2015-2017 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Portions of this software were developed by SRI International and the
 * University of Cambridge Computer Laboratory under DARPA/AFRL contract
 * FA8750-10-C-0237 ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Portions of this software were developed by the University of Cambridge
 * Computer Laboratory as part of the CTSRD Project, with support from the
 * UK Higher Education Innovation Fund (HEIF).
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/* MODIFIED */

#ifndef _RISCV_RISCVREG_H_
#define _RISCV_RISCVREG_H_

#if __riscv_xlen == 64
#define SCAUSE_INTR (1ul << 63)
#else
#define SCAUSE_INTR (1 << 31)
#endif
#define SCAUSE_CODE (~SCAUSE_INTR)
#define SCAUSE_INST_MISALIGNED 0
#define SCAUSE_INST_ACCESS_FAULT 1
#define SCAUSE_ILLEGAL_INSTRUCTION 2
#define SCAUSE_BREAKPOINT 3
#define SCAUSE_LOAD_MISALIGNED 4
#define SCAUSE_LOAD_ACCESS_FAULT 5
#define SCAUSE_STORE_MISALIGNED 6
#define SCAUSE_STORE_ACCESS_FAULT 7
#define SCAUSE_ECALL_USER 8
#define SCAUSE_ECALL_SUPERVISOR 9
#define SCAUSE_INST_PAGE_FAULT 12
#define SCAUSE_LOAD_PAGE_FAULT 13
#define SCAUSE_STORE_PAGE_FAULT 15

#define SSTATUS_UIE (1 << 0)
#define SSTATUS_SIE (1 << 1)
#define SSTATUS_UPIE (1 << 4)
#define SSTATUS_SPIE (1 << 5)
#define SSTATUS_SPIE_SHIFT 5
#define SSTATUS_SPP_SHIFT 8
#define SSTATUS_SPP_USER (0 << SSTATUS_SPP_SHIFT)
#define SSTATUS_SPP_SUPV (1 << SSTATUS_SPP_SHIFT)
#define SSTATUS_SPP_MASK (1 << SSTATUS_SPP_SHIFT)
#define SSTATUS_FS_SHIFT 13
#define SSTATUS_FS_OFF (0x0 << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_INITIAL (0x1 << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_CLEAN (0x2 << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_DIRTY (0x3 << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_MASK (0x3 << SSTATUS_FS_SHIFT)
#define SSTATUS_XS_SHIFT 15
#define SSTATUS_XS_MASK (0x3 << SSTATUS_XS_SHIFT)
#define SSTATUS_SUM (1 << 18)
#if __riscv_xlen == 64
#define SSTATUS_SD (1ul << 63)
#else
#define SSTATUS_SD (1 << 31)
#endif

#define SIE_USIE (1 << 0)
#define SIE_SSIE (1 << 1)
#define SIE_UTIE (1 << 4)
#define SIE_STIE (1 << 5)
#define SIE_UEIE (1 << 8)
#define SIE_SEIE (1 << 9)

/* Note: sip register has no SIP_STIP bit in Spike simulator */
#define SIP_USIP (1 << 0)
#define SIP_SSIP (1 << 1)
#define SIP_UTIP (1 << 4)
#define SIP_STIP (1 << 5)
#define SIP_UEIP (1 << 8)
#define SIP_SEIP (1 << 9)

#if __riscv_xlen == 64
#define SATP_PPN_S 0
#define SATP_PPN_M (0xfffffffffff << SATP_PPN_S)
#define SATP_ASID_S 44
#define SATP_ASID_M (0xffff << SATP_ASID_S)
#define SATP_MODE_S 60
#define SATP_MODE_M (0xfULL << SATP_MODE_S)
#define SATP_MODE_SV39 (8ULL << SATP_MODE_S)
#define SATP_MODE_SV48 (9ULL << SATP_MODE_S)
#define MAX_ASID 0xffff
#else
#define SATP_PPN_S 0
#define SATP_PPN_M (0x3fffff << SATP_PPN_S)
#define SATP_ASID_S 22
#define SATP_ASID_M (0x1ff << SATP_ASID_S)
#define SATP_MODE_S 31
#define SATP_MODE_M (0x1 << SATP_MODE_S)
#define SATP_MODE_SV32 (1 << SATP_MODE_S)
#define MAX_ASID 0x01ff
#endif

#define XLEN __riscv_xlen
#define XLEN_BYTES (XLEN / 8)
#define INSN_SIZE 4
#define INSN_C_SIZE 2

#define csr_swap(csr, val)                                                     \
  ({                                                                           \
    __asm __volatile("csrrw %0, " #csr ", %1" : "=r"(val) : "r"(val));         \
    val;                                                                       \
  })

#define csr_write(csr, val) __asm __volatile("csrw " #csr ", %0" ::"r"(val))

#define csr_set(csr, val) __asm __volatile("csrs " #csr ", %0" ::"r"(val))

#define csr_clear(csr, val) __asm __volatile("csrc " #csr ", %0" ::"r"(val))

#define csr_read(csr)                                                          \
  ({                                                                           \
    u_long val;                                                                \
    __asm __volatile("csrr %0, " #csr : "=r"(val));                            \
    val;                                                                       \
  })

#if __riscv_xlen == 32
#define csr_read64(csr)                                                        \
  ({                                                                           \
    uint64_t val;                                                              \
    uint32_t high, low;                                                        \
    __asm __volatile("1: "                                                     \
                     "csrr t0, " #csr "h\n"                                    \
                     "csrr %0, " #csr "\n"                                     \
                     "csrr %1, " #csr "h\n"                                    \
                     "bne t0, %1, 1b"                                          \
                     : "=r"(low), "=r"(high)                                   \
                     :                                                         \
                     : "t0");                                                  \
    val = (low | ((uint64_t)high << 32));                                      \
    val;                                                                       \
  })
#else
#define csr_read64(csr) ((uint64_t)csr_read(csr))
#endif

#endif /* !_RISCV_RISCVREG_H_ */
