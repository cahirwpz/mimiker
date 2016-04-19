#ifndef __SYS_INTERRUPTS_H__
#define __SYS_INTERRUPTS_H__

#include <common.h>

/* Initializes and enables interrupts. */
void intr_init();

#define intr_disable() __extension__ ({ asm("di"); })
#define intr_enable() __extension__ ({ asm("ei"); })

#endif /* __SYS_INTERRUPTS_H__ */
