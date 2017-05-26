#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include <common.h>

typedef uint32_t systime_t; /* kept in miliseconds */

typedef struct timeval {
  time_t tv_sec;       /* seconds */
  suseconds_t tv_usec; /* microseconds */
} timeval_t;

typedef struct timespec {
  time_t tv_sec; /* seconds */
  long tv_nsec;  /* and nanoseconds */
} timespec_t;

#define TIMEVAL(fp)                                                            \
  (timeval_t) {                                                                \
    .tv_sec = (long)((fp)*1000000L) / 1000000L,                                \
    .tv_usec = (long)((fp)*1000000L) % 1000000L                                \
  }

#define TIMEVAL_PAIR(sec, usec)                                                \
  (timeval_t) {                                                                \
    .tv_sec = (sec), .tv_usec = (usec)                                         \
  }

static inline timeval_t st2tv(systime_t st) {
  return TIMEVAL_PAIR(st / 1000, st % 1000);
}

static inline systime_t tv2st(timeval_t tv) {
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* Operations on timevals. */
static inline void timeval_clear(timeval_t *tvp) {
  *tvp = (timeval_t){.tv_sec = 0, .tv_usec = 0};
}

static inline bool timeval_isset(timeval_t *tvp) {
  return tvp->tv_sec || tvp->tv_usec;
}

#define timeval_cmp(tvp, uvp, cmp)                                             \
  (((tvp)->tv_sec == (uvp)->tv_sec) ? (((tvp)->tv_usec)cmp((uvp)->tv_usec))    \
                                    : (((tvp)->tv_sec)cmp((uvp)->tv_sec)))

static inline void timeval_add(timeval_t *tvp, timeval_t *uvp, timeval_t *vvp) {
  vvp->tv_sec = tvp->tv_sec + uvp->tv_sec;
  vvp->tv_usec = tvp->tv_usec + uvp->tv_usec;
  if (vvp->tv_usec >= 1000000) {
    vvp->tv_sec++;
    vvp->tv_usec -= 1000000;
  }
}

static inline void timeval_sub(timeval_t *tvp, timeval_t *uvp, timeval_t *vvp) {
  vvp->tv_sec = tvp->tv_sec - uvp->tv_sec;
  vvp->tv_usec = tvp->tv_usec - uvp->tv_usec;
  if (vvp->tv_usec < 0) {
    vvp->tv_sec--;
    vvp->tv_usec += 1000000;
  }
}

#endif /* !_SYS_TIME_H_ */
