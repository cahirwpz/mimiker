#include <stdio.h>
#include <stdarg.h>
#include <low/_stdio.h>
#include "uart_raw.h"

int __low_kprintf (ReadWriteInfo *rw, const void *src, size_t len)
{
    uart_putnstr(len,src);
    return len;
}

/* Print a formatted string to UART. */
int kprintf(const char *fmt, ...)
{
    int ret;
    ReadWriteInfo rw;
    va_list ap;
    va_start(ap, fmt);
    rw.m_fnptr = __low_kprintf;
    ret = _format_parser(&rw, fmt, &ap, 0);
    va_end(ap);
    return ret;
}
