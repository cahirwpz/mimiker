#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include <sys/types.h>

#define CLK_TCK 100 /* system clock ticks per second */

typedef struct tm {
  int tm_sec;          /* seconds after the minute [0-61] */
  int tm_min;          /* minutes after the hour [0-59] */
  int tm_hour;         /* hours since midnight [0-23] */
  int tm_mday;         /* day of the month [1-31] */
  int tm_mon;          /* months since January [0-11] */
  int tm_year;         /* years since 1900 */
  int tm_wday;         /* days since Sunday [0-6] */
  int tm_yday;         /* days since January 1 [0-365] */
  int tm_isdst;        /* Daylight Savings Time flag */
  long tm_gmtoff;      /* offset from UTC in seconds */
  const char *tm_zone; /* timezone abbreviation */
} tm_t;

typedef struct timespec {
  time_t tv_sec; /* seconds */
  long tv_nsec;  /* and nanoseconds */
} timespec_t;

typedef struct bintime {
  time_t sec;    /* second */
  uint64_t frac; /* a fraction of second */
} bintime_t;

#define BINTIME(fp)                                                            \
  (bintime_t) {                                                                \
    .sec = (time_t)__builtin_floor(fp),                                        \
    .frac = (uint64_t)((fp - __builtin_floor(fp)) * (1ULL << 63) * 2)          \
  }

#define TIMESPEC(fp)                                                           \
  (timespec_t) {                                                               \
    .tv_sec = (uint64_t)((fp)*1000000000L) / 1000000000L,                      \
    .tv_nsec = (uint64_t)((fp)*1000000000L) % 1000000000L                      \
  }

#define HZ2BT(hz)                                                              \
  (bintime_t) {                                                                \
    .sec = 0, .frac = ((1ULL << 63) / (hz)) << 1                               \
  }

static inline systime_t ts2st(timespec_t ts) {
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static inline timespec_t bt2ts(bintime_t bt) {
  uint32_t nsec = ((uint64_t)1000000000 * (uint32_t)(bt.frac >> 32)) >> 32;
  return (timespec_t){.tv_sec = bt.sec, .tv_nsec = nsec};
}

static inline bintime_t ts2bt(timespec_t ts) {
  /* 1ns = (2^64) / 1000000000 */
  const uint64_t bintime_scale_ns = 18446744073ULL;
  uint64_t frac = (uint64_t)ts.tv_nsec * bintime_scale_ns;
  return (bintime_t){.sec = ts.tv_sec, .frac = frac};
#undef BINTIME_SCALE_NS
}

/* Operations on bintime. */
#define bintime_cmp(a, b, cmp)                                                 \
  (((a).sec == (b).sec) ? (((a).frac)cmp((b).frac)) : (((a).sec)cmp((b).sec)))

static inline void bintime_add(bintime_t *bt, bintime_t *bt2) {
  uint64_t old_frac = bt->frac;
  bt->frac += bt2->frac;
  if (old_frac > bt->frac)
    bt->sec++;
  bt->sec += bt2->sec;
}

static inline void bintime_sub(bintime_t *bt, bintime_t *bt2) {
  uint64_t old_frac = bt->frac;
  bt->frac -= bt2->frac;
  if (old_frac < bt->frac)
    bt->sec--;
  bt->sec -= bt2->sec;
}

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

typedef enum clockid { CLOCK_MONOTONIC = 1, CLOCK_REALTIME = 2 } clockid_t;

/* Operations on timespecs. */
#define timespecclear(tsp) (tsp)->tv_sec = (time_t)((tsp)->tv_nsec = 0L)
#define timespecisset(tsp) ((tsp)->tv_sec || (tsp)->tv_nsec)
#define timespeccmp(tsp, usp, cmp)                                             \
  (((tsp)->tv_sec == (usp)->tv_sec) ? ((tsp)->tv_nsec cmp(usp)->tv_nsec)       \
                                    : ((tsp)->tv_sec cmp(usp)->tv_sec))
#define timespecadd(tsp, usp, vsp)                                             \
  {                                                                            \
    (vsp)->tv_sec = (tsp)->tv_sec + (usp)->tv_sec;                             \
    (vsp)->tv_nsec = (tsp)->tv_nsec + (usp)->tv_nsec;                          \
    if ((vsp)->tv_nsec >= 1000000000L) {                                       \
      (vsp)->tv_sec++;                                                         \
      (vsp)->tv_nsec -= 1000000000L;                                           \
    }                                                                          \
  }
#define timespecsub(tsp, usp, vsp)                                             \
  {                                                                            \
    (vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;                             \
    (vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;                          \
    if ((vsp)->tv_nsec < 0) {                                                  \
      (vsp)->tv_sec--;                                                         \
      (vsp)->tv_nsec += 1000000000L;                                           \
    }                                                                          \
  }
#define timespec2ns(x) (((uint64_t)(x)->tv_sec) * 1000000000L + (x)->tv_nsec)

static inline int timespec_isset(timespec_t *tsp) {
  return tsp->tv_sec || tsp->tv_nsec;
}

#ifdef _KERNEL

/* Time measured from the start of system. */
bintime_t binuptime(void);
timespec_t nanouptime(void);

/* UTC/POSIX time */
bintime_t bintime(void);
timespec_t nanotime(void);

/* System time is measured in ticks (1[ms] by default),
 * and is maintained by system clock. */
systime_t getsystime(void);

int do_clock_gettime(clockid_t clk, timespec_t *tp);

int do_clock_nanosleep(clockid_t clk, int flags, const timespec_t *rqtp,
                       timespec_t *rmtp);

#else /* _KERNEL */

typedef struct timeval {
  time_t tv_sec;       /* seconds */
  suseconds_t tv_usec; /* microseconds */
} timeval_t;

/*
 * Names of the interval timers, and structure defining a timer setting.
 */
#define ITIMER_REAL 0
#define ITIMER_VIRTUAL 1
#define ITIMER_PROF 2
#define ITIMER_MONOTONIC 3

struct itimerval {
  struct timeval it_interval; /* timer interval */
  struct timeval it_value;    /* current value */
};

/* Not used */
#define TIMEVAL(fp)                                                            \
  (timeval_t) {                                                                \
    .tv_sec = (long)((fp)*1000000L) / 1000000L,                                \
    .tv_usec = (long)((fp)*1000000L) % 1000000L                                \
  }

/* Not used */
static inline timeval_t st2tv(systime_t st) {
  return (timeval_t){.tv_sec = st / 1000, .tv_usec = st % 1000};
}

static inline timeval_t ts2tv(timespec_t ts) {
  return (timeval_t){.tv_sec = ts.tv_sec, .tv_usec = ts.tv_nsec / 1000};
}

/* Not used */
static inline systime_t tv2st(timeval_t tv) {
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* Not used */
static inline timeval_t bt2tv(bintime_t bt) {
  uint32_t usec = ((uint64_t)1000000 * (uint32_t)(bt.frac >> 32)) >> 32;
  return (timeval_t){.tv_sec = bt.sec, .tv_usec = usec};
}

/* Operations on timevals. */
#define timerclear(tvp) (tvp)->tv_sec = (tvp)->tv_usec = 0L
#define timerisset(tvp) ((tvp)->tv_sec || (tvp)->tv_usec)
#define timercmp(tvp, uvp, cmp)                                                \
  (((tvp)->tv_sec == (uvp)->tv_sec) ? ((tvp)->tv_usec cmp(uvp)->tv_usec)       \
                                    : ((tvp)->tv_sec cmp(uvp)->tv_sec))
#define timeradd(tvp, uvp, vvp)                                                \
  {                                                                            \
    (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;                             \
    (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;                          \
    if ((vvp)->tv_usec >= 1000000) {                                           \
      (vvp)->tv_sec++;                                                         \
      (vvp)->tv_usec -= 1000000;                                               \
    }                                                                          \
  }
#define timersub(tvp, uvp, vvp)                                                \
  {                                                                            \
    (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;                             \
    (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;                          \
    if ((vvp)->tv_usec < 0) {                                                  \
      (vvp)->tv_sec--;                                                         \
      (vvp)->tv_usec += 1000000;                                               \
    }                                                                          \
  }

/* Not used */
static inline void timeval_clear(timeval_t *tvp) {
  *tvp = (timeval_t){.tv_sec = 0, .tv_usec = 0};
}

/* Not used */
static inline int timeval_isset(timeval_t *tvp) {
  return tvp->tv_sec || tvp->tv_usec;
}

#define timeval_cmp(tvp, uvp, cmp)                                             \
  (((tvp)->tv_sec == (uvp)->tv_sec) ? (((tvp)->tv_usec)cmp((uvp)->tv_usec))    \
                                    : (((tvp)->tv_sec)cmp((uvp)->tv_sec)))
/* Probably trash */
static inline timeval_t timeval_add(timeval_t *tvp, timeval_t *uvp) {
  timeval_t res = {.tv_sec = tvp->tv_sec + uvp->tv_sec,
                   .tv_usec = tvp->tv_usec + uvp->tv_usec};
  if (res.tv_usec >= 1000000) {
    res.tv_sec++;
    res.tv_usec -= 1000000;
  }
  return res;
}
/* Probably trash */
static inline timeval_t timeval_sub(timeval_t *tvp, timeval_t *uvp) {
  timeval_t res = {.tv_sec = tvp->tv_sec - uvp->tv_sec,
                   .tv_usec = tvp->tv_usec - uvp->tv_usec};
  if (res.tv_usec < 0) {
    res.tv_sec--;
    res.tv_usec += 1000000;
  }
  return res;
}

int nanosleep(timespec_t *rqtp, timespec_t *rmtp);

int gettimeofday(timeval_t *tp, void *tzp);

int clock_gettime(clockid_t clk, timespec_t *tp);

int clock_nanosleep(clockid_t clk, int flags, const timespec_t *rqtp,
                    timespec_t *rmtp);

int setitimer(int, const struct itimerval *__restrict,
              struct itimerval *__restrict);

#endif /* !_KERNEL */

#endif /* !_SYS_TIME_H_ */
