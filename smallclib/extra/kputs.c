#include <stdio.h>
#include <string.h>
#include "uart_raw.h"

/* Write a character string and a trailing newline to UART. */
int kputs (const char *s)
{
    unsigned int len = strlen(s);
    uart_putnstr(len, s);
    uart_putnstr(1, "\n");
    return len;
}/* puts */
