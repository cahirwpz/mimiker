#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stdint.h>

typedef int64_t realtime_t;

realtime_t clock_get();
void clock(realtime_t ms);

#endif // __CLOCK_H__
