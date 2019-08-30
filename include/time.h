#ifndef _TIME_H_
#define _TIME_H_

#include <sys/cdefs.h>
#include <sys/time.h>

typedef unsigned int clock_t;

typedef struct __state *timezone_t;

#define CLOCKS_PER_SEC 100

__BEGIN_DECLS
char *asctime(const struct tm *);
clock_t clock(void);
char *ctime(const time_t *);
double difftime(time_t, time_t);
struct tm *gmtime(const time_t *);
struct tm *localtime(const time_t *);
time_t time(time_t *);
time_t mktime(struct tm *);
size_t strftime(char *__restrict, size_t, const char *__restrict,
                const struct tm *__restrict)
  __attribute__((__format__(__strftime__, 3, 0)));
char *asctime_r(const struct tm *__restrict, char *__restrict);
__END_DECLS

#ifndef __LOCALE_T_DECLARED
typedef struct _locale *locale_t;
#define __LOCALE_T_DECLARED
#endif

__BEGIN_DECLS
time_t mktime_z(timezone_t __restrict, struct tm *__restrict);
time_t timelocal_z(timezone_t __restrict, struct tm *);
size_t strftime_lz(timezone_t __restrict, char *__restrict, size_t,
                   const char *__restrict, const struct tm *__restrict,
                   locale_t) __attribute__((__format__(__strftime__, 4, 0)));
size_t strftime_z(timezone_t __restrict, char *__restrict, size_t,
                  const char *__restrict, const struct tm *__restrict)
  __attribute__((__format__(__strftime__, 4, 0)));
__END_DECLS

#endif /* !_TIME_H_ */
