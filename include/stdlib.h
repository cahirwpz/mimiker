#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <sys/cdefs.h>

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
const char *getenv(const char *);
int putenv(char *);
void *malloc(size_t);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
int rand(void);
void *realloc(void *, size_t);
void srand(unsigned);
double strtod(const char *__restrict, char **__restrict);
long strtol(const char *__restrict, char **__restrict, int);
unsigned long strtoul(const char *__restrict, char **__restrict, int);
int system(const char *);
__END_DECLS

/* The Open Group Base Specifications, Issue 6; IEEE Std 1003.1-2001 (POSIX) */
int setenv(const char *, const char *, int);
int unsetenv(const char *);

#endif /* !_STDLIB_H_ */
