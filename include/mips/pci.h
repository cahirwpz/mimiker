#ifndef _SYS_MIPS_PCI_H_
#define _SYS_MIPS_PCI_H_

#include <mips/gt64120.h>

#define PCI0_CFG_ADDR_R GT_R(GT_PCI0_CFG_ADDR)
#define PCI0_CFG_DATA_R GT_R(GT_PCI0_CFG_DATA)

#define PCI0_CFG_REG_SHIFT 2
#define PCI0_CFG_FUNCT_SHIFT 8
#define PCI0_CFG_DEV_SHIFT 11
#define PCI0_CFG_BUS_SHIFT 16
#define PCI0_CFG_ENABLE 0x80000000

#define PCI0_CFG_REG(dev, funct, reg)                                          \
  (((dev) << PCI0_CFG_DEV_SHIFT) | ((funct) << PCI0_CFG_FUNCT_SHIFT) |         \
   ((reg) << PCI0_CFG_REG_SHIFT))

#endif /* !_SYS_MIPS_PCI_H_ */
