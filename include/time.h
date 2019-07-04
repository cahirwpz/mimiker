#ifndef _TIME_H_
#define _TIME_H_

#include <sys/cdefs.h>

typedef unsigned int clock_t;

#define CLOCKS_PER_SEC 100

struct tm {
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
};

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
__END_DECLS

#endif /* !_TIME_H_ */
