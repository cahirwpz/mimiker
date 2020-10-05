/*-
 * Copyright (c) 2014 Andrew Turner
 * Copyright (c) 2014-2015 The FreeBSD Foundation
 * All rights reserved.
 *
 * This software was developed by Andrew Turner under
 * sponsorship from the FreeBSD Foundation.
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
 */

#ifndef _MACHINE_PTE_H_
#define _MACHINE_PTE_H_

#include <sys/types.h>

typedef uint8_t asid_t;
typedef uint64_t pte_t;
typedef uint64_t pde_t;

#define PAGE_SHIFT 12
#define ASID_SHIFT 48
#define MAX_ASID 0xFF

/* Block and Page attributes */
#define ATTR_MASK_H UINT64_C(0xfff0000000000000)
#define ATTR_MASK_L UINT64_C(0x0000000000000fff)
#define ATTR_MASK (ATTR_MASK_H | ATTR_MASK_L)
/* Bits 58:55 are reserved for software */
#define ATTR_SW_RW (1UL << 57)
#define ATTR_SW_MANAGED (1UL << 56)
#define ATTR_SW_WIRED (1UL << 55)
#define ATTR_UXN (1UL << 54)
#define ATTR_PXN (1UL << 53)
#define ATTR_XN (ATTR_PXN | ATTR_UXN)
#define ATTR_CONTIGUOUS (1UL << 52)
#define ATTR_DBM (1UL << 51)
#define ATTR_nG (1 << 11)
#define ATTR_AF (1 << 10)
#define ATTR_SH(x) ((x) << 8)
#define ATTR_SH_MASK ATTR_SH(3)
#define ATTR_SH_NS 0 /* Non-shareable */
#define ATTR_SH_OS 2 /* Outer-shareable */
#define ATTR_SH_IS 3 /* Inner-shareable */
#define ATTR_AP_RW_BIT (1 << 7)
#define ATTR_AP(x) ((x) << 6)
#define ATTR_AP_MASK ATTR_AP(3)
#define ATTR_AP_RW (0 << 1)
#define ATTR_AP_RO (1 << 1)
#define ATTR_AP_USER (1 << 0)
#define ATTR_NS (1 << 5)
#define ATTR_IDX(x) ((x) << 2)
#define ATTR_IDX_MASK (7 << 2)

#define ATTR_DEVICE_MEM 0
#define ATTR_NORMAL_MEM_NC 1
#define ATTR_NORMAL_MEM_WB 2
#define ATTR_NORMAL_MEM_WT 3

#define ATTR_S2_S2AP(x) ((x) << 6)
#define ATTR_S2_S2AP_MASK 3
#define ATTR_S2_S2AP_READ 1
#define ATTR_S2_S2AP_WRITE 2

/* Level 0 table, 512GiB per entry */
#define L0_SHIFT 39
#define L0_SIZE (1ul << L0_SHIFT)
#define L0_OFFSET (L0_SIZE - 1ul)
#define L0_INVAL 0x0 /* An invalid address */
                     /* 0x1 Level 0 doesn't support block translation */
                     /* 0x2 also marks an invalid address */
#define L0_TABLE 0x3 /* A next-level table */

/* Level 1 table, 1GiB per entry */
#define L1_SHIFT 30
#define L1_SIZE (1 << L1_SHIFT)
#define L1_OFFSET (L1_SIZE - 1)
#define L1_INVAL L0_INVAL
#define L1_BLOCK 0x1
#define L1_TABLE L0_TABLE

/* Level 2 table, 2MiB per entry */
#define L2_SHIFT 21
#define L2_SIZE (1 << L2_SHIFT)
#define L2_OFFSET (L2_SIZE - 1)
#define L2_INVAL L1_INVAL
#define L2_BLOCK L1_BLOCK
#define L2_TABLE L1_TABLE

#define L2_BLOCK_MASK UINT64_C(0xffffffe00000)

/* Level 3 table, 4KiB per entry */
#define L3_SHIFT 12
#define L3_SIZE (1 << L3_SHIFT)
#define L3_OFFSET (L3_SIZE - 1)
#define L3_SHIFT 12
#define L3_INVAL 0x0
/* 0x1 is reserved */
/* 0x2 also marks an invalid address */
#define L3_PAGE 0x3

#define L0_ENTRIES_SHIFT 9
#define L0_ENTRIES (1 << L0_ENTRIES_SHIFT)
#define L0_ADDR_MASK (L0_ENTRIES - 1)

#define Ln_ENTRIES_SHIFT 9
#define Ln_ENTRIES (1 << Ln_ENTRIES_SHIFT)
#define Ln_ADDR_MASK (Ln_ENTRIES - 1)
#define Ln_TABLE_MASK ((1 << 12) - 1)

#define Ln_VALID __BIT(0)
#define Ln_TABLE_PA __BITS(47, 12)
#define L1_BLOCK_OA __BITS(47, 30) /* 1GB */
#define L2_BLOCK_OA __BITS(47, 21) /* 2MB */
#define L3_PAGE_OA __BITS(47, 12)  /* 4KB */

#define L0_INDEX(va) (((va) >> L0_SHIFT) & L0_ADDR_MASK)
#define L1_INDEX(va) (((va) >> L1_SHIFT) & Ln_ADDR_MASK)
#define L2_INDEX(va) (((va) >> L2_SHIFT) & Ln_ADDR_MASK)
#define L3_INDEX(va) (((va) >> L3_SHIFT) & Ln_ADDR_MASK)

#endif /* !_MACHINE_PTE_H_ */
