#ifndef _SYS_CLOCK_H_
#define _SYS_CLOCK_H_

#include <time.h>

timeval_t get_uptime(void);

void clock(systime_t ms);

#endif /* !_SYS_CLOCK_H_ */
