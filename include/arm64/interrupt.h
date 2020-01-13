#ifndef _ARM64_INTERRUPT_H_
#define _ARM64_INTERRUPT_H_

/*! \brief Disables local CPU interrupts. */
void cpu_intr_disable(void);

/*! \brief Enables local CPU interrupts. */
void cpu_intr_enable(void);

/*! \brief Check if local CPU interrupts are disabled. */
bool cpu_intr_disabled(void);

#endif /* !_ARM64_INTERRUPT_H_ */
