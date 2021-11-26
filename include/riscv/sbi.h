/*-
 * Copyright (c) 2016-2017 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 * Copyright (c) 2019 Mitchell Horne <mhorne@FreeBSD.org>
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

/*
 * Documentation available at
 * https://github.com/riscv/riscv-sbi-doc/blob/master/riscv-sbi.adoc
 */

#ifndef _RISCV_SBI_H_
#define _RISCV_SBI_H_

/* SBI Specification Version */
#define SBI_SPEC_VERS_MAJOR_OFFSET 24
#define SBI_SPEC_VERS_MAJOR_MASK (0x7F << SBI_SPEC_VERS_MAJOR_OFFSET)
#define SBI_SPEC_VERS_MINOR_OFFSET 0
#define SBI_SPEC_VERS_MINOR_MASK (0xFFFFFF << SBI_SPEC_VERS_MINOR_OFFSET)

/* SBI Implementation IDs */
#define SBI_IMPL_ID_BBL 0
#define SBI_IMPL_ID_OPENSBI 1
#define SBI_IMPL_ID_XVISOR 2
#define SBI_IMPL_ID_KVM 3
#define SBI_IMPL_ID_RUSTSBI 4
#define SBI_IMPL_ID_DIOSIX 5

/* SBI Error Codes */
#define SBI_SUCCESS 0
#define SBI_ERR_FAILURE -1
#define SBI_ERR_NOT_SUPPORTED -2
#define SBI_ERR_INVALID_PARAM -3
#define SBI_ERR_DENIED -4
#define SBI_ERR_INVALID_ADDRESS -5
#define SBI_ERR_ALREADY_AVAILABLE -6

/* SBI Base Extension */
#define SBI_EXT_ID_BASE 0x10
#define SBI_BASE_GET_SPEC_VERSION 0
#define SBI_BASE_GET_IMPL_ID 1
#define SBI_BASE_GET_IMPL_VERSION 2
#define SBI_BASE_PROBE_EXTENSION 3
#define SBI_BASE_GET_MVENDORID 4
#define SBI_BASE_GET_MARCHID 5
#define SBI_BASE_GET_MIMPID 6

/* Timer (TIME) Extension */
#define SBI_EXT_ID_TIME 0x54494D45
#define SBI_TIME_SET_TIMER 0

/* IPI (IPI) Extension */
#define SBI_EXT_ID_IPI 0x735049
#define SBI_IPI_SEND_IPI 0

/* RFENCE (RFNC) Extension */
#define SBI_EXT_ID_RFNC 0x52464E43
#define SBI_RFNC_REMOTE_FENCE_I 0
#define SBI_RFNC_REMOTE_SFENCE_VMA 1
#define SBI_RFNC_REMOTE_SFENCE_VMA_ASID 2
#define SBI_RFNC_REMOTE_HFENCE_GVMA_VMID 3
#define SBI_RFNC_REMOTE_HFENCE_GVMA 4
#define SBI_RFNC_REMOTE_HFENCE_VVMA_ASID 5
#define SBI_RFNC_REMOTE_HFENCE_VVMA 6

/* Hart State Management (HSM) Extension */
#define SBI_EXT_ID_HSM 0x48534D
#define SBI_HSM_HART_START 0
#define SBI_HSM_HART_STOP 1
#define SBI_HSM_HART_STATUS 2
#define SBI_HSM_STATUS_STARTED 0
#define SBI_HSM_STATUS_STOPPED 1
#define SBI_HSM_STATUS_START_PENDING 2
#define SBI_HSM_STATUS_STOP_PENDING 3

/* Legacy Extensions */
#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_SEND_IPI 4
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8

/* SBI Implementation-Specific Definitions */
#define OPENSBI_VERSION_MAJOR_OFFSET 16
#define OPENSBI_VERSION_MINOR_MASK 0xFFFF

long sbi_probe_extension(unsigned long id);

/* Timer extension functions. */
void sbi_set_timer(uint64_t val);

/* IPI extension functions. */
void sbi_send_ipi(const unsigned long *hart_mask);

/* RFENCE extension functions. */
void sbi_remote_fence_i(const unsigned long *hart_mask);
void sbi_remote_sfence_vma(const unsigned long *hart_mask, unsigned long start,
                           unsigned long size);
void sbi_remote_sfence_vma_asid(const unsigned long *hart_mask,
                                unsigned long start, unsigned long size,
                                unsigned long asid);

/* Hart State Management extension functions. */

/*
 * Start execution on the specified hart at physical address start_addr. The
 * register a0 will contain the hart's ID, and a1 will contain the value of
 * priv.
 */
int sbi_hsm_hart_start(unsigned long hart, unsigned long start_addr,
                       unsigned long priv);

/*
 * Stop execution on the current hart. Interrupts should be disabled, or this
 * function may return.
 */
void sbi_hsm_hart_stop(void);

/*
 * Get the execution status of the specified hart. The status will be one of:
 *  - SBI_HSM_STATUS_STARTED
 *  - SBI_HSM_STATUS_STOPPED
 *  - SBI_HSM_STATUS_START_PENDING
 *  - SBI_HSM_STATUS_STOP_PENDING
 */
int sbi_hsm_hart_status(unsigned long hart);

/* Legacy extension functions. */
void sbi_console_putchar(int ch);
int sbi_console_getchar(void);
void sbi_shutdown(void);

/* Initialize SBI module. */
void sbi_init(void);

#endif /* !_RISCV_SBI_H_ */
