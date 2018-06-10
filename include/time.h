#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include <common.h>

typedef uint32_t systime_t; /* kept in miliseconds */

typedef struct tm {
  int tm_sec;     /* seconds after the minute [0-60] */
  int tm_min;     /* minutes after the hour [0-59] */
  int tm_hour;    /* hours since midnight [0-23] */
  int tm_mday;    /* day of the month [1-31] */
  int tm_mon;     /* months since January [0-11] */
  int tm_year;    /* years since 1900 */
  int tm_wday;    /* days since Sunday [0-6] */
  int tm_yday;    /* days since January 1 [0-365] */
  int tm_isdst;   /* Daylight Savings Time flag */
  long tm_gmtoff; /* offset from UTC in seconds */
  char *tm_zone;  /* timezone abbreviation */
} tm_t;

typedef struct timeval {
  time_t tv_sec;       /* seconds */
  suseconds_t tv_usec; /* microseconds */
} timeval_t;

typedef struct timespec {
  time_t tv_sec; /* seconds */
  long tv_nsec;  /* and nanoseconds */
} timespec_t;

typedef struct bintime {
  time_t sec;    /* second */
  uint64_t frac; /* a fraction of second */
} bintime_t;

#define TIMEVAL(fp)                                                            \
  (timeval_t) {                                                                \
    .tv_sec = (long)((fp)*1000000L) / 1000000L,                                \
    .tv_usec = (long)((fp)*1000000L) % 1000000L                                \
  }

#define BINTIME(fp)                                                            \
  (bintime_t) {                                                                \
    .sec = (time_t)__builtin_floor(fp),                                        \
    .frac = (uint64_t)((fp - __builtin_floor(fp)) * (1ULL << 64))              \
  }

#define HZ2BT(hz)                                                              \
  (bintime_t) {                                                                \
    .sec = 0, .frac = ((1ULL << 63) / (hz)) << 1                               \
  }

static inline timeval_t st2tv(systime_t st) {
  return (timeval_t){.tv_sec = st / 1000, .tv_usec = st % 1000};
}

static inline systime_t tv2st(timeval_t tv) {
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline timeval_t bt2tv(bintime_t bt) {
  uint32_t usec = ((uint64_t)1000000 * (uint32_t)(bt.frac >> 32)) >> 32;
  return (timeval_t){.tv_sec = bt.sec, .tv_usec = usec};
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
  timeval_t res = {.tv_sec = tvp->tv_sec + uvp->tv_sec,
                   .tv_usec = tvp->tv_usec + uvp->tv_usec};
  if (res.tv_usec >= 1000000) {
    res.tv_sec++;
    res.tv_usec -= 1000000;
  }
  return res;
}

static inline timeval_t timeval_sub(timeval_t *tvp, timeval_t *uvp) {
  timeval_t res = {.tv_sec = tvp->tv_sec - uvp->tv_sec,
                   .tv_usec = tvp->tv_usec - uvp->tv_usec};
  if (res.tv_usec < 0) {
    res.tv_sec--;
    res.tv_usec += 1000000;
  }
  return res;
}

/* Operations on bintime. */
#define bintime_cmp(a, b, cmp)                                                 \
  (((a).sec == (b).sec) ? (((a).frac)cmp((b).frac)) : (((a).sec)cmp((b).sec)))

static inline void bintime_add_frac(bintime_t *bt, uint64_t x) {
  uint64_t old_frac = bt->frac;
  bt->frac += x;
  if (old_frac > bt->frac)
    bt->sec++;
}

static inline bintime_t bintime_mul(const bintime_t bt, uint32_t x) {
  uint64_t p1 = (bt.frac & 0xffffffffULL) * x;
  uint64_t p2 = (bt.frac >> 32) * x + (p1 >> 32);
  return (bintime_t){.sec = bt.sec * x + (p2 >> 32),
                     .frac = (p2 << 32) | (p1 & 0xffffffffULL)};
}

/* XXX: Do not use this function, it'll get removed. */
timeval_t get_uptime(void);

/* Get high-fidelity time measured from the start of system. */
bintime_t getbintime(void);

/* System time is measured in ticks (1[ms] by default),
 * and is maintained by system clock. */
systime_t getsystime(void);

/* XXX: Do not use this function, it'll get removed.
 * Raw access to cpu internal timer. */
timeval_t getcputime(void);

#endif /* !_SYS_TIME_H_ */
