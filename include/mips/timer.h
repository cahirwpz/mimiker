#ifndef _MIPS_TIMER_H_
#define _MIPS_TIMER_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

void mips_timer_init(void);

#endif /* !_MIPS_TIMER_H_ */
