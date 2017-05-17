#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include <stdbool.h>
#include <stdint.h>

typedef long realtime_t;

typedef struct timeval {
  long tv_sec;  /* seconds */
  long tv_usec; /* microseconds */
} timeval_t;

/* Operations on timevals. */
static inline void timerclear(timeval_t *tvp) {
  *tvp = (timeval_t){.tv_sec = 0, .tv_usec = 0};
}

static inline bool timerisset(timeval_t *tvp) {
  return tvp->tv_sec || tvp->tv_usec;
}

#define timercmp(tvp, uvp, cmp)                                                \
  (((tvp)->tv_sec == (uvp)->tv_sec) ? (((tvp)->tv_usec)cmp((uvp)->tv_usec))    \
                                    : (((tvp)->tv_sec)cmp((uvp)->tv_sec)))

static inline void timeradd(timeval_t *tvp, timeval_t *uvp, timeval_t *vvp) {
  vvp->tv_sec = tvp->tv_sec + uvp->tv_sec;
  vvp->tv_usec = tvp->tv_usec + uvp->tv_usec;
  if (vvp->tv_usec >= 1000000) {
    vvp->tv_sec++;
    vvp->tv_usec -= 1000000;
  }
}

static inline void timersub(timeval_t *tvp, timeval_t *uvp, timeval_t *vvp) {
  vvp->tv_sec = tvp->tv_sec - uvp->tv_sec;
  vvp->tv_usec = tvp->tv_usec - uvp->tv_usec;
  if (vvp->tv_usec < 0) {
    vvp->tv_sec--;
    vvp->tv_usec += 1000000;
  }
}

#endif /* !_SYS_TIME_H_ */
