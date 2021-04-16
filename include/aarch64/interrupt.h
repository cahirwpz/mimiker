#ifndef _AARCH64_INTERRUPT_H_
#define _AARCH64_INTERRUPT_H_

/*! \brief Disables local CPU interrupts. */
void cpu_intr_disable(void) __no_instrument_function;

/*! \brief Enables local CPU interrupts. */
void cpu_intr_enable(void) __no_instrument_function;

/*! \brief Check if local CPU interrupts are disabled. */
bool cpu_intr_disabled(void) __no_instrument_function;

#endif /* !_AARCH64_INTERRUPT_H_ */
