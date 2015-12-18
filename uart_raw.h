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

/* Transmits a single byte via UART1. */
int uart_putc(int c);
/* Transmits a string and a trailing newline via UART1. */
int uart_puts(const char* str);
/* Transmits n bytes via UART1. */
int uart_write(const char* str, size_t n);
/* Transmits a 32it number in hex format via UART1. */
/* TODO: Now that kprintf provides formatting wrapper for UART, this
   procedure is no longer needed. */
void uart_puthex(unsigned value);

/* Receives a single byte from UART.
 * This function blocks execution until a byte is available.
 */
unsigned char uart_getch();

/* This function is a convenience for displaying register values
 * over UART.
 * @param prefix: A string with the name of the displayed register.
 *                The name will be outputted before the value.
 * @param value: An unsigned 32bit value to display.
 */
void uart_printreg(const char* prefix, unsigned value);

#endif // UART_RAW_H
