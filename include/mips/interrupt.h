#ifndef _MIPS_INTERRUPT_H_
#define _MIPS_INTERRUPT_H_

#ifdef _MACHDEP

typedef enum {
  MIPS_SWINT0,
  MIPS_SWINT1,
  MIPS_HWINT0,
  MIPS_HWINT1,
  MIPS_HWINT2,
  MIPS_HWINT3,
  MIPS_HWINT4,
  MIPS_HWINT5,
} mips_intr_t;

#define MIPS_NIRQ 8 /* count of MIPS processor interrupt requests */

#endif /* !_MACHDEP */

#endif /* !_MIPS_INTERRUPT_H_ */
