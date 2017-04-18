#include <common.h>
#include <console.h>
#include <linker_set.h>
#include <ns16550.h>
#include <mips/malta.h>
#include <mips/uart_cbus.h>

#define CBUS_UART_R(x)                                                         \
  *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(MALTA_CBUS_UART) + (x))

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

static void cbus_uart_init(console_t *dev __unused) {
  LCR |= LCR_DLAB;
  DLM = 0;
  DLL = 1; /* 115200 */
  LCR &= ~LCR_DLAB;

  IER = 0;
  FCR = 0;
  LCR = LCR_8BITS; /* 8-bit data, no parity */
}

static void cbus_uart_putc(console_t *dev __unused, int c) {
  /* Wait for transmitter hold register empty. */
  while (!(LSR & LSR_THRE))
    ;

again:
  /* Send byte. */
  THR = c;

  /* Wait for transmitter hold register empty. */
  while (!(LSR & LSR_THRE))
    ;

  if (c == '\n') {
    c = '\r';
    goto again;
  }
}

static int cbus_uart_getc(console_t *dev __unused) {
  /* Wait until receive data available. */
  while (!(LSR & LSR_RXRDY))
    ;

  return RBR;
}

CONSOLE_DEFINE(cbus_uart, 10);
