#ifndef _INTTYPES_H_
#define _INTTYPES_H_

#include <stdint.h>
#include <sys/inttypes.h>

#include <_locale.h>

__BEGIN_DECLS
intmax_t strtoimax(const char *__restrict, char **__restrict, int);
uintmax_t strtoumax(const char *__restrict, char **__restrict, int);

intmax_t strtoi(const char *__restrict, char **__restrict, int, intmax_t,
                intmax_t, int *);
uintmax_t strtou(const char *__restrict, char **__restrict, int, uintmax_t,
                 uintmax_t, int *);

intmax_t strtoimax_l(const char *__restrict, char **__restrict, int, locale_t);
uintmax_t strtoumax_l(const char *__restrict, char **__restrict, int, locale_t);
__END_DECLS

#endif /* !_INTTYPES_H_ */
