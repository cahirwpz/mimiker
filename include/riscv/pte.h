/*-
 * Copyright (c) 2014 Andrew Turner
 * Copyright (c) 2015-2018 Ruslan Bukin <br@bsdpad.com>
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

#ifndef _RISCV_PTE_H_
#define _RISCV_PTE_H_

typedef uint32_t pd_entry_t; /* page directory entry */
typedef uint32_t pt_entry_t; /* page table entry */
typedef uint32_t pn_t;       /* page number */

/* Level 0 table, 4MiB per entry */
#define L0_SHIFT 22
#define L0_SIZE (1 << L0_SHIFT)
#define L0_OFFSET (L0_SIZE - 1)

/* Level 1 table, 4096B per entry */
#define L1_SHIFT 12
#define L1_SIZE (1 << L1_SHIFT)
#define L1_OFFSET (L1_SIZE - 1)

#define Ln_ENTRIES_SHIFT 10
#define Ln_ENTRIES (1 << Ln_ENTRIES_SHIFT)
#define Ln_ADDR_MASK (Ln_ENTRIES - 1)

#define L0_INDEX(va) (((va) >> L0_SHIFT) & Ln_ADDR_MASK)
#define L1_INDEX(va) (((va) >> L1_SHIFT) & Ln_ADDR_MASK)

/* Bits 9:8 are reserved for software */
#define PTE_SW_MANAGED (1 << 9)
#define PTE_SW_WIRED (1 << 8)
#define PTE_D (1 << 7) /* Dirty */
#define PTE_A (1 << 6) /* Accessed */
#define PTE_G (1 << 5) /* Global */
#define PTE_U (1 << 4) /* User */
#define PTE_X (1 << 3) /* Execute */
#define PTE_W (1 << 2) /* Write */
#define PTE_R (1 << 1) /* Read */
#define PTE_V (1 << 0) /* Valid */
#define PTE_RWX (PTE_R | PTE_W | PTE_X)
#define PTE_RX (PTE_R | PTE_X)
#define PTE_KERN (PTE_V | PTE_G)

#define PTE_PPN0_S 10
#define PTE_PPN1_S 20
#define PTE_SIZE 4

#define PAGE_SHIFT 12

#define PA_TO_PTE(pa) (((pa) >> PAGE_SHIFT) << PTE_PPN0_S)
#define PTE_TO_PA(pte) (((pte) >> PTE_PPN0_S) << PAGE_SHIFT)

#endif /* !_RISCV_PTE_H_ */
