#include <common.h>
#include <mips.h>
#include <ns16550.h>
#include <malta.h>
#include <uart_cbus.h>

#define CBUS_UART_R(x) \
  *(volatile uint8_t*)(MIPS_PHYS_TO_KSEG1(MALTA_CBUS_UART) + (x))

#define RBR CBUS_UART_R(0x00) /* Receiver Buffer, read-only, DLAB = 0 */
#define THR CBUS_UART_R(0x00) /* Transmitter Holding, write-only, DLAB = 0 */
#define DLL CBUS_UART_R(0x00) /* Divisor Latch LSB, read-write, DLAB = 1 */
#define IER CBUS_UART_R(0x08) /* Interrupt Enable, read-write, DLAB = 0 */
#define DLM CBUS_UART_R(0x08) /* Divisor Latch LSB read-write, DLAB = 1 */
#define IIR CBUS_UART_R(0x10) /* Interrupt Identification, read-only */
#define FCR CBUS_UART_R(0x10) /* FIFO Control, write-only */
#define LCR CBUS_UART_R(0x18) /* Line Control, read-write */
#define MCR CBUS_UART_R(0x20) /* Modem Control, read-write */
#define LSR CBUS_UART_R(0x28) /* Line Status, read-only */
#define MSR CBUS_UART_R(0x30) /* Modem Status, read-only */

void uart_init() {
  LCR |= LCR_DLAB;
  DLM = 0; DLL = 1; /* 115200 */
  LCR &= ~LCR_DLAB;

  IER = 0;
  FCR = 0;
  LCR = LCR_8BITS; /* 8-bit data, no parity */
}

int uart_putc(int c) {
  /* Wait for transmitter hold register empty. */
  while (! (LSR & LSR_THRE));

again:
  /* Send byte. */
  THR = c;

  /* Wait for transmitter hold register empty. */
  while (! (LSR & LSR_THRE));

  if (c == '\n') {
    c = '\r';
    goto again;
  }
  return c;
}

int uart_puts(const char *str) {
  int n = 0;
  while (*str) {
    uart_putc(*str++);
    n++;
  }
  uart_putc('\n');
  return n + 1;
}

int uart_write(const char *str, size_t n) {
  while (n--) uart_putc(*str++);
  return n;
}

unsigned char uart_getch() {
  /* Wait until receive data available. */
  while (! (LSR & LSR_RXRDY));

  return RBR;
}
