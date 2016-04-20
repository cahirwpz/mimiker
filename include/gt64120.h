#ifndef __GT64120_H__
#define __GT64120_H__

#include <malta.h>

#define GT_R(x) *((volatile uint32_t *) \
    (MIPS_PHYS_TO_KSEG1(MALTA_CORECTRL_BASE + (x))))

/* PCI Internal Register Map */
#define GT_PCI0_CFG_ADDR 0xcf8
#define GT_PCI0_CFG_DATA 0xcfc

#endif
