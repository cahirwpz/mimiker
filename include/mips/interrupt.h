#ifndef _MIPS_INTERRUPT_H_
#define _MIPS_INTERRUPT_H_

/*! \file mips/interrupt.h */

#include <stdbool.h>

/*! \brief Disables interrupts by setting SR.IE to 0.
 *
 * \warning Do not use these if you don't know what you're doing!
 * Use \a intr_disable instead.
 *
 * \see intr_enable
 * \see intr_disable
 */
void cpu_intr_disable(void);

/*! \brief Enables interrupts by setting SR.IE to 1. */
void cpu_intr_enable(void);

/*! \brief Check if interrupts are disabled.
 *
 * Interrupts are enabled when SR.IE = 1 and SR.EXL = 0 and SR.ERL = 0,
 * according to MIPS documentation.
 *
 * The kernel leaves Exception (EXL) or Error Level (ERL) as soon as possible,
 * hence we consider exceptions to be disabled if and only if SR.IE = 0.
 */
bool cpu_intr_disabled(void);

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

#endif /* !_MACHDEP */

#endif /* !_MIPS_INTERRUPT_H_ */
