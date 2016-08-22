#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <common.h>

/* This file contains declarations for standard functions that are
   imported from smallclib and exposed for use within the kernel, as
   well as several additional utilities. */

/* ============================================
   These functions are imported from smallclib. */
void *memset (void *m, int c, size_t n);
void *memcpy (void *dst0, const void *src0, size_t len0);
size_t strlen(const char *str);
void bzero(void *s, size_t n);
void qsort(void *base, size_t num, size_t width,
           int (*comp)(const void *, const void *));
/* ============================================ */


/* The snprintf has a custom implementation which cannot format
   floating-point numbers, see snprintf.c */
int snprintf (char *str, size_t size, const char *fmt, ...)
  __attribute__((format (printf, 3, 4)));

/* Write a formatted string to UART.
 * Equivalent to standard printf. */
int kprintf(const char *fmt, ...)
  __attribute__((format (printf, 1, 2)));

/* Write a character string and a trailing newline to UART.
 * Equivalent to standard puts */
int kputs (const char *s);

/* Write a character to UART.
 * Equivalent to standard putchar */
int kputchar (int c);

#endif /* __STDLIB_H__ */
