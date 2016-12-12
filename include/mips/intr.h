#ifndef _MIPS_INTR_H_
#define _MIPS_INTR_H_

/* Do not use these! Consider critical_enter / critical_leave instead. */
#define intr_disable() __extension__({ asm("di"); })
#define intr_enable() __extension__({ asm("ei"); })

#endif
