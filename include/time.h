#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include <common.h>

typedef uint32_t systime_t; /* kept in miliseconds */

typedef struct timeval {
  time_t tv_sec;       /* seconds */
  suseconds_t tv_usec; /* microseconds */
} timeval_t;

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

static inline timeval_t timeval_add(timeval_t *tvp, timeval_t *uvp) {
  timeval_t ret =
    TIMEVAL_PAIR(tvp->tv_sec + uvp->tv_sec, tvp->tv_usec + uvp->tv_usec);
  if (ret.tv_usec >= 1000000) {
    ret.tv_sec++;
    ret.tv_usec -= 1000000;
  }
  return ret;
}

static inline timeval_t timeval_sub(timeval_t *tvp, timeval_t *uvp) {
  timeval_t ret =
    TIMEVAL_PAIR(tvp->tv_sec - uvp->tv_sec, tvp->tv_usec - uvp->tv_usec);
  if (ret.tv_usec < 0) {
    ret.tv_sec--;
    ret.tv_usec += 1000000;
  }
  return ret;
}

#endif /* !_SYS_TIME_H_ */
