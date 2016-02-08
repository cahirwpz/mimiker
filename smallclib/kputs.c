#include <libkern.h>
#include "uart_raw.h"

/* Write a character string and a trailing newline to UART. */
int kputs (const char *s) {
  return uart_puts(s);
}/* puts */
