#ifndef _TIME_H_
#define _TIME_H_

#include <sys/cdefs.h>
#include <sys/time.h>
#include <sys/_clock.h>
#include <_locale.h>

typedef struct __state *timezone_t;

#define CLOCKS_PER_SEC 100

__BEGIN_DECLS
char *asctime(const struct tm *);
clock_t clock(void);
char *ctime(const time_t *);
double difftime(time_t, time_t);
struct tm *gmtime(const time_t *);
struct tm *gmtime_r(const time_t *__restrict, struct tm *__restrict);
struct tm *localtime(const time_t *);
struct tm *localtime_r(const time_t *__restrict, struct tm *__restrict);
time_t time(time_t *);
time_t mktime(struct tm *);
char *strptime(const char * __restrict, const char * __restrict,
    struct tm * __restrict);
size_t strftime(char *__restrict, size_t, const char *__restrict,
                const struct tm *__restrict)
  __attribute__((__format__(__strftime__, 3, 0)));
char *asctime_r(const struct tm *__restrict, char *__restrict);

time_t mktime_z(timezone_t __restrict, struct tm *__restrict);
time_t timelocal_z(timezone_t __restrict, struct tm *);
size_t strftime_lz(timezone_t __restrict, char *__restrict, size_t,
                   const char *__restrict, const struct tm *__restrict,
                   locale_t) __attribute__((__format__(__strftime__, 4, 0)));
size_t strftime_z(timezone_t __restrict, char *__restrict, size_t,
                  const char *__restrict, const struct tm *__restrict)
  __attribute__((__format__(__strftime__, 4, 0)));
char *strptime_l(const char * __restrict, const char * __restrict,
    struct tm * __restrict, locale_t);
timezone_t tzalloc(const char *);
void tzfree(const timezone_t);
void tzset(void);
long tzgetgmtoff(const timezone_t, int);
__END_DECLS

#endif /* !_TIME_H_ */
