#ifndef _WCHAR_H_
#define _WCHAR_H_

#include <stdint.h>

#ifndef __LOCALE_T_DECLARED
typedef struct _locale *locale_t;
#define __LOCALE_T_DECLARED
#endif

extern size_t __mb_cur_max;
#define MB_CUR_MAX __mb_cur_max

#ifndef WEOF
#define WEOF ((wint_t)-1)
#endif

/*
 * mbstate_t is an opaque object to keep conversion state, during multibyte
 * stream conversions.  The content must not be referenced by user programs.
 */
typedef union {
  int64_t __mbstateL; /* for alignment */
  char __mbstate8[128];
} __mbstate_t;

typedef __mbstate_t mbstate_t;

struct tm;

__BEGIN_DECLS
wint_t btowc(int);
size_t mbrlen(const char *__restrict, size_t, mbstate_t *__restrict);
size_t mbrtowc(wchar_t *__restrict, const char *__restrict, size_t,
               mbstate_t *__restrict);
int mbsinit(const mbstate_t *);
size_t mbsrtowcs(wchar_t *__restrict, const char **__restrict, size_t,
                 mbstate_t *__restrict);
size_t wcrtomb(char *__restrict, wchar_t, mbstate_t *__restrict);
wchar_t *wcscat(wchar_t *__restrict, const wchar_t *__restrict);
wchar_t *wcschr(const wchar_t *, wchar_t);
int wcscmp(const wchar_t *, const wchar_t *);
int wcscoll(const wchar_t *, const wchar_t *);
wchar_t *wcscpy(wchar_t *__restrict, const wchar_t *__restrict);
size_t wcscspn(const wchar_t *, const wchar_t *);
size_t wcsftime(wchar_t *__restrict, size_t, const wchar_t *__restrict,
                const struct tm *__restrict);
size_t wcslen(const wchar_t *);
wchar_t *wcsncat(wchar_t *__restrict, const wchar_t *__restrict, size_t);
int wcsncmp(const wchar_t *, const wchar_t *, size_t);
wchar_t *wcsncpy(wchar_t *__restrict, const wchar_t *__restrict, size_t);
size_t wcsnlen(const wchar_t *, size_t);
wchar_t *wcspbrk(const wchar_t *, const wchar_t *);
wchar_t *wcsrchr(const wchar_t *, wchar_t);
size_t wcsrtombs(char *__restrict, const wchar_t **__restrict, size_t,
                 mbstate_t *__restrict);
size_t wcsspn(const wchar_t *, const wchar_t *);
wchar_t *wcsstr(const wchar_t *, const wchar_t *);
wchar_t *wcstok(wchar_t *__restrict, const wchar_t *__restrict,
                wchar_t **__restrict);
size_t wcsxfrm(wchar_t *, const wchar_t *, size_t);
wchar_t *wcswcs(const wchar_t *, const wchar_t *);
wchar_t *wmemchr(const wchar_t *, wchar_t, size_t);
int wmemcmp(const wchar_t *, const wchar_t *, size_t);
wchar_t *wmemcpy(wchar_t *__restrict, const wchar_t *__restrict, size_t);
wchar_t *wmemmove(wchar_t *, const wchar_t *, size_t);
wchar_t *wmemset(wchar_t *, wchar_t, size_t);
__END_DECLS

__BEGIN_DECLS
size_t wcrtomb_l(char *s, wchar_t wc, mbstate_t *ps, locale_t loc);
size_t wcsrtombs_l(char *s, const wchar_t **ppwcs, size_t n, mbstate_t *ps,
                   locale_t loc);
__END_DECLS

__BEGIN_DECLS
size_t mbrtowc_l(wchar_t *__restrict, const char *__restrict, size_t,
                 mbstate_t *__restrict, locale_t);
int wctob_l(wint_t, locale_t);
__END_DECLS

#endif /* !_WCHAR_H_ */
