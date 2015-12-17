#ifndef __custom_STDIO_H__
#define __custom_STDIO_H__

/* This is a wrapper header for stdio.h.  It appends serveral
 * kernel-based string output functions to the standard stdio.h
 * header.
 */

#include_next <stdio.h>

/* Write a formatted string to UART.
 * Equivalent to standard printf. */
int kprintf(const char *fmt, ...);
/* Write a character string and a trailing newline to UART.
 * Equivalent to standard putss */
int kputs (const char *s);
/* Print an error message to UART.
 * Equivalent to standard perror. */
void kperror (const char *s);

#endif // __custom_STDIO_H__
