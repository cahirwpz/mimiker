/* See LICENSE file for copyright and license details. */
#include <sys/types.h>

#include <regex.h>
#include <stddef.h>
#include <stdio.h>

#include "arg.h"
#include "compat.h"

#define UTF8_POINT(c) (((c) & 0xc0) != 0x80)

#undef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#undef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#undef LIMIT
#define LIMIT(x, a, b)  (x) = (x) < (a) ? (a) : (x) > (b) ? (b) : (x)

#define LEN(x) (sizeof (x) / sizeof *(x))

extern char *argv0;

void *ecalloc(size_t, size_t);
void *emalloc(size_t);
void *erealloc(void *, size_t);
#undef reallocarray
void *reallocarray(void *, size_t, size_t);
void *ereallocarray(void *, size_t, size_t);
char *estrdup(const char *);
char *estrndup(const char *, size_t);
void *encalloc(int, size_t, size_t);
void *enmalloc(int, size_t);
void *enrealloc(int, void *, size_t);
void *enreallocarray(int, void *, size_t, size_t);
char *enstrdup(int, const char *);
char *enstrndup(int, const char *, size_t);

void enfshut(int, FILE *, const char *);
void efshut(FILE *, const char *);
int  fshut(FILE *, const char *);

void enprintf(int, const char *, ...);
void eprintf(const char *, ...);
void weprintf(const char *, ...);

double estrtod(const char *);

#undef strcasestr
#define strcasestr xstrcasestr
char *strcasestr(const char *, const char *);

#undef strlcat
#define strlcat xstrlcat
size_t strlcat(char *, const char *, size_t);
size_t estrlcat(char *, const char *, size_t);
#undef strlcpy
#define strlcpy xstrlcpy
size_t strlcpy(char *, const char *, size_t);
size_t estrlcpy(char *, const char *, size_t);

#undef strsep
#define strsep xstrsep
char *strsep(char **, const char *);

/* regex */
int enregcomp(int, regex_t *, const char *, int);
int eregcomp(regex_t *, const char *, int);

/* io */
ssize_t writeall(int, const void *, size_t);
int concat(int, const char *, int, const char *);

/* misc */
void enmasse(int, char **, int (*)(const char *, const char *, int));
void fnck(const char *, const char *, int (*)(const char *, const char *, int), int);
mode_t getumask(void);
char *humansize(off_t);
mode_t parsemode(const char *, mode_t, mode_t);
off_t parseoffset(const char *);
void putword(FILE *, const char *);
#undef strtonum
#define strtonum xstrtonum
long long strtonum(const char *, long long, long long, const char **);
long long enstrtonum(int, const char *, long long, long long);
long long estrtonum(const char *, long long, long long);
size_t unescape(char *);
int mkdirp(const char *, mode_t, mode_t);
#undef memmem
#define memmem xmemmem
void *memmem(const void *, size_t, const void *, size_t);
