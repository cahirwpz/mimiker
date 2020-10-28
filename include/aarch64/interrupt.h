#ifndef _AARCH64_INTERRUPT_H_
#define _AARCH64_INTERRUPT_H_

/*! \brief Disables local CPU interrupts. */
void cpu_intr_disable(void);

/*! \brief Enables local CPU interrupts. */
void cpu_intr_enable(void);

/*! \brief Check if local CPU interrupts are disabled. */
bool cpu_intr_disabled(void);

typedef struct intr_handler intr_handler_t;

void init_aarch64_intr(void);

void aarch64_intr_setup(intr_handler_t *handler, unsigned irq);

void aarch64_intr_teardown(intr_handler_t *handler);

#endif /* !_AARCH64_INTERRUPT_H_ */
