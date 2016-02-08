#ifndef UART_RAW_H
#define UART_RAW_H

#include <stddef.h>

/* Raw UART interface. Useful for debugging.
 * A higher-level solution will have to be implemented eventually,
 *  but temporarily this should be fine.
 */


/* This procedure initializes UART. It must be called (once) before
 * any other uart_* functions. */
void uart_init();

/* Transmits a single byte via UART1. Returns the transmitted
   character. */
int uart_putc(int c);
/* Transmits a string and a trailing newline via UART1. Returns the
   number of bytes transmitted. */
int uart_puts(const char *str);
/* Transmits n bytes via UART1. Returns the number of bytes
   transmitted. */
int uart_write(const char *str, size_t n);

/* Receives a single byte from UART.
 * This function blocks execution until a byte is available.
 */
unsigned char uart_getch();

#endif // UART_RAW_H
