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

#include <stdint.h>

#if __riscv_xlen == 64
typedef uint64_t pd_entry_t; /* page directory entry */
typedef uint64_t pt_entry_t; /* page table entry */
#else
typedef uint32_t pd_entry_t; /* page directory entry */
typedef uint32_t pt_entry_t; /* page table entry */
#endif

typedef uint16_t asid_t; /* address space identifier */

#if __riscv_xlen == 64
#define L0_SHIFT 30 /* level 0 table, 1GiB per entry */
#define L1_SHIFT 21 /* level 1 table, 2MiB per entry */
#define L2_SHIFT 12 /* level 2 table, 4096B per entry */
#define Ln_ENTRIES_SHIFT 9
#else
#define L0_SHIFT 22 /* level 0 table, 4MiB per entry */
#define L1_SHIFT 12 /* level 1 table, 4096B per entry */
#define Ln_ENTRIES_SHIFT 10
#endif

#define Ln_ENTRIES (1 << Ln_ENTRIES_SHIFT)
#define Ln_ADDR_MASK (Ln_ENTRIES - 1)

#define L0_SIZE (1 << L0_SHIFT)
#define L0_OFFSET (L0_SIZE - 1)
#define L0_INDEX(va) (((va) >> L0_SHIFT) & Ln_ADDR_MASK)

#define L1_SIZE (1 << L1_SHIFT)
#define L1_OFFSET (L1_SIZE - 1)
#define L1_INDEX(va) (((va) >> L1_SHIFT) & Ln_ADDR_MASK)

#ifdef L2_SHIFT
#define L2_SIZE (1 << L2_SHIFT)
#define L2_OFFSET (L2_SIZE - 1)
#define L2_INDEX(va) (((va) >> L2_SHIFT) & Ln_ADDR_MASK)
#endif /* !L2_SHIFT */

/* Bits 9:8 are reserved for software. */
#define PTE_SW_WRITE (1 << 9)
#define PTE_SW_READ (1 << 8)
#define PTE_SW_FLAGS (PTE_SW_WRITE | PTE_SW_READ)
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
#define PTE_KERN                                                               \
  (PTE_SW_WRITE | PTE_SW_READ | PTE_D | PTE_A | PTE_G | PTE_W | PTE_R | PTE_V)
#define PTE_KERN_RO (PTE_SW_READ | PTE_A | PTE_G | PTE_R | PTE_V)
#define PTE_PROT_MASK (PTE_SW_FLAGS | PTE_D | PTE_A | PTE_RWX | PTE_V)

#define PTE_PPN0_S 10
#define PAGE_SHIFT 12

#define PA_TO_PTE(pa) (((pa) >> PAGE_SHIFT) << PTE_PPN0_S)
#define PTE_TO_PA(pte) (((pte) >> PTE_PPN0_S) << PAGE_SHIFT)

#define VALID_PDE_P(pde) (((pde)&PTE_V) != 0)

#define VALID_PTE_P(pte) ((pte) != 0)

#define LEAF_PTE_P(pte) (((pte) & (PTE_X | PTE_W | PTE_R)) != 0)

#endif /* !_RISCV_PTE_H_ */
