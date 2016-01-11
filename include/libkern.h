#ifndef __LIBKERN_H__
#define __LIBKERN_H__
#include <stddef.h>

/* This file contains declarations for standard functions that are
   imported from smallclib and exposed for use within the kernel, as
   well as several additional utilities. */

/* ============================================
   These functions are imported from smallclib. */
void *memset (void *m, int c, size_t n);
void *memcpy (void *dst0, const void *src0, size_t len0);
size_t strlen(const char *str);
void bzero(void *s, size_t n);
/* ============================================ */


/* The snprintf has a custom implementation which cannot format
   floating-point numbers, see snprintf.c */
int snprintf (char *str, size_t size, const char *fmt, ...);

/* Write a formatted string to UART.
 * Equivalent to standard printf. */
int kprintf(const char *fmt, ...);
/* Write a character string and a trailing newline to UART.
 * Equivalent to standard putss */
int kputs (const char *s);

#endif // __LIBKERN_H__
