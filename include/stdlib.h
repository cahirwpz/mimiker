#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <sys/types.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define RAND_MAX 0x7fffffff

__BEGIN_DECLS
__noreturn void _Exit(int);
__noreturn void abort(void);
int atexit(void (*)(void));
double atof(const char *);
int atoi(const char *);
long atol(const char *);
void *calloc(size_t, size_t);
__noreturn void exit(int);
void free(void *);
char *getenv(const char *);
int putenv(char *);
void *malloc(size_t);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
int rand(void);
void *realloc(void *, size_t);
void srand(unsigned);
double strtod(const char *__restrict, char **__restrict);
float strtof(const char *__restrict, char **__restrict);
long double strtold(const char *__restrict, char **__restrict);
long strtol(const char *__restrict, char **__restrict, int);
unsigned long strtoul(const char *__restrict, char **__restrict, int);
int system(const char *);

const char *getprogname(void) __constfunc;

/* The Open Group Base Specifications, Issue 6; IEEE Std 1003.1-2001 (POSIX) */
int setenv(const char *, const char *, int);
int unsetenv(const char *);

/*
 * X/Open Portability Guide >= Issue 4 Version 2
 */
char *mkdtemp(char *);
int mkstemp(char *);
char *mktemp(char *);

#ifndef __LOCALE_T_DECLARED
typedef struct _locale *locale_t;
#define __LOCALE_T_DECLARED
#endif

double strtod_l(const char *__restrict, char **__restrict, locale_t);
float strtof_l(const char *__restrict, char **__restrict, locale_t);
long double strtold_l(const char *__restrict, char **__restrict, locale_t);
long strtol_l(const char *__restrict, char **__restrict, int, locale_t);
unsigned long strtoul_l(const char *__restrict, char **__restrict, int,
                        locale_t);

size_t _mb_cur_max_l(locale_t);
#define MB_CUR_MAX_L(loc) _mb_cur_max_l(loc)

int mblen_l(const char *, size_t, locale_t);
size_t mbstowcs_l(wchar_t *__restrict, const char *__restrict, size_t,
                  locale_t);
int wctomb_l(char *, wchar_t, locale_t);
int mbtowc_l(wchar_t *__restrict, const char *__restrict, size_t, locale_t);
size_t wcstombs_l(char *__restrict, const wchar_t *__restrict, size_t,
                  locale_t);

__END_DECLS

void *alloca(size_t); /* built-in for gcc */

#endif /* !_STDLIB_H_ */
