#ifndef _SYS_CPU_H_
#define _SYS_CPU_H_

/*! \file sys/cpu.h */

#include <stdbool.h>
#include <sys/cdefs.h>

/*! \brief Disables local CPU interrupts.
 *
 * \warning Do not use these if you don't know what you're doing!
 * Use \a intr_disable instead.
 *
 * \see intr_enable
 * \see intr_disable
 */
void cpu_intr_disable(void) __no_profile;

/*! \brief Enables local CPU interrupts. */
void cpu_intr_enable(void) __no_profile;

/*! \brief Check if local CPU interrupts are disabled. */
bool cpu_intr_disabled(void) __no_profile;

#endif /* !_SYS_CPU_H_ */
