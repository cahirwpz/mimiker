#ifndef _SYS_CLOCK_H_
#define _SYS_CLOCK_H_

#include <time.h>

timeval_t clock_get();
void clock(realtime_t ms);

#endif /* !_SYS_CLOCK_H_ */
