#include <libkern.h>
#include <stdarg.h>
#include <low/_stdio.h>
#include <uart_cbus.h>

int __low_kprintf (ReadWriteInfo *rw __attribute__((unused)), const void *src,
                   size_t len) {
  return uart_write(src, len);
}

/* Print a formatted string to UART. */
int kprintf(const char *fmt, ...) {
  int ret;
  ReadWriteInfo rw;
  va_list ap;
  va_start(ap, fmt);
  rw.m_fnptr = __low_kprintf;
  ret = _format_parser_int(&rw, fmt, &ap);
  va_end(ap);
  return ret;
}
