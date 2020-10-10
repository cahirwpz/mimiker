#ifndef _AARCH64_TIMER_H_
#define _AARCH64_TIMER_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

void init_arm_timer(void);

#endif /* !_AARCH64_TIMER_H_ */
