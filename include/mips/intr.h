#ifndef _MIPS_INTR_H_
#define _MIPS_INTR_H_

typedef struct intr_handler intr_handler_t;
typedef struct exc_frame exc_frame_t;

/* Do not use these! Consider critical_enter / critical_leave instead. */
#define intr_disable() __extension__({ asm("di"); })
#define intr_enable() __extension__({ asm("ei"); })

void mips_intr_init();
void mips_intr_handler(exc_frame_t *frame);
void mips_intr_setup(intr_handler_t *ih, unsigned irq);
void mips_intr_teardown(intr_handler_t *ih);

#endif
