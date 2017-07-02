#include <common.h>
#include <console.h>
#include <linker_set.h>
#include <dev/ns16550reg.h>
#include <mips/malta.h>
#include <bus.h>

RESOURCE_DECLARE(cbus_uart);

static uint8_t in(unsigned offset) {
  return bus_space_read_1(cbus_uart, offset);
}

static void out(unsigned offset, uint8_t value) {
  bus_space_write_1(cbus_uart, offset, value);
}

static void set(unsigned offset, uint8_t mask) {
  out(offset, in(offset) | mask);
}

static void clr(unsigned offset, uint8_t mask) {
  out(offset, in(offset) & ~mask);
}

static bool is_set(unsigned offset, uint8_t mask) {
  return in(offset)&mask;
}

static void cbus_uart_init(console_t *dev __unused) {
  set(LCR, LCR_DLAB);
  out(DLM, 0);
  out(DLL, 1); /* 115200 */
  clr(LCR, LCR_DLAB);

  out(IER, 0);
  out(FCR, 0);
  out(LCR, LCR_8BITS); /* 8-bit data, no parity */
}

static void cbus_uart_putc(console_t *dev __unused, int c) {
  /* Wait for transmitter hold register empty. */
  while (!is_set(LSR, LSR_THRE))
    ;

again:
  /* Send byte. */
  out(THR, c);

  /* Wait for transmitter hold register empty. */
  while (!is_set(LSR, LSR_THRE))
    ;

  if (c == '\n') {
    c = '\r';
    goto again;
  }
}

static int cbus_uart_getc(console_t *dev __unused) {
  /* Wait until receive data available. */
  while (!is_set(LSR, LSR_RXRDY))
    ;

  return in(RBR);
}

static console_t cbus_uart_console = {.cn_init = cbus_uart_init,
                                      .cn_getc = cbus_uart_getc,
                                      .cn_putc = cbus_uart_putc,
                                      .cn_prio = 10};

CONSOLE_ADD(cbus_uart_console);
