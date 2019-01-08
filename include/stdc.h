#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <common.h>

/*
 * This file contains declarations for subset of C standard library functions.
 * Implementation is imported from various places (mainly smallclib and OpenBSD)
 * and exposed for use within the kernel.
 *
 * There're extra utilities here as well.
 */

/* ctype.h function prototypes */
int isalnum(int);
int isalpha(int);
int iscntrl(int);
int isdigit(int);
int isgraph(int);
int islower(int);
int isprint(int);
int ispunct(int);
int isspace(int);
int isupper(int);
int isxdigit(int);
int tolower(int);
int toupper(int);
int isblank(int);
int isascii(int);
int toascii(int);

/* stdio.h function prototypes */

/* The snprintf has a custom implementation which cannot format
   floating-point numbers, see snprintf.c */
int snprintf(char *str, size_t size, const char *fmt, ...)
  __attribute__((format(printf, 3, 4)));
int sscanf(const char *str, const char *fmt, ...)
  __attribute__((format(scanf, 2, 3)));

/* stdlib.h function prototypes */
long strtol(const char *nptr, char **endptr, int base);
long strntol(const char *nptr, size_t len, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
unsigned long strntoul(const char *nptr, size_t len, char **endptr, int base);
void qsort(void *base, size_t num, size_t width,
           int (*comp)(const void *, const void *));
int rand_r(unsigned *seedp);

/* strings.h function prototypes */

void bzero(void *b, size_t length);
void *memchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *s1, const void *s2, size_t n);
void *memset(void *dst, int c, size_t n);
char *strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
size_t strcspn(const char *s1, const char *s2);
size_t strlcat(char *dst, const char *src, size_t dsize);
size_t strlcpy(char *dst, const char *src, size_t dsize);
size_t strlen(const char *str);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strnlen(const char *str, size_t maxlen);
char *strrchr(const char *s, int c);
char *strsep(char **stringp, const char *delim);
size_t strspn(const char *s1, const char *s2);

/* Write a formatted string to default console. */
int kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Write a character string and a trailing newline to default console. */
int kputs(const char *s);

/* Write a character to default console. */
int kputchar(int c);

#endif /* __STDLIB_H__ */
