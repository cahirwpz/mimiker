/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Florent Kermarrec <florent@enjoy-digital.fr>
 * Copyright (c) 2020 Dolu1990 <charles.papon.90@gmail.com>
 *
 */

/* MODIFIED */

#include <stdint.h>

#define UART_EV_TX	0x1
#define UART_EV_RX	0x2

/* NOTE: keep this synchronized with the CSR map. */
#define UART_CSR_BASE		0xf0001000L

#define UART_RXTX_CSR		(UART_CSR_BASE + 0x000)
#define UART_TXFULL_CSR		(UART_CSR_BASE + 0x004)
#define UART_EV_PENDING_CSR	(UART_CSR_BASE + 0x010)
#define UART_TXEMPTY_CSR	(UART_CSR_BASE + 0x018)
#define UART_RXFULL_CSR		(UART_CSR_BASE + 0x01c)

#define MMPTR(a) (*((volatile uint32_t *)(a)))

static inline void csr_write_simple(unsigned long v, unsigned long a)
{
	MMPTR(a) = v;
}

static inline unsigned long csr_read_simple(unsigned long a)
{
	return MMPTR(a);
}

static inline uint8_t uart_rxtx_read(void) {
	return csr_read_simple(UART_RXTX_CSR);
}

static inline void uart_rxtx_write(uint8_t v) {
	csr_write_simple(v, UART_RXTX_CSR);
}

static inline uint8_t uart_txfull_read(void) {
	return csr_read_simple(UART_TXFULL_CSR);
}

static inline uint8_t uart_rxempty_read(void) {
	return csr_read_simple(UART_TXEMPTY_CSR);
}

static inline void uart_ev_pending_write(uint8_t v) {
	csr_write_simple(v, UART_EV_PENDING_CSR);
}

static inline uint8_t uart_rxfull_read(void) {
	return csr_read_simple(UART_RXFULL_CSR);
}

void vex_putc(char c){
	while (uart_txfull_read());
	uart_rxtx_write(c);
	uart_ev_pending_write(UART_EV_TX);
}

int vex_getc(void){
	char c;
	if (uart_rxempty_read())
		return -1;
	c = uart_rxtx_read();
	uart_ev_pending_write(UART_EV_RX);
	return c;
}
