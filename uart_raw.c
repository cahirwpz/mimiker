#include <libkern.h>
#include "pic32mz.h"
#include "uart_raw.h"
#include "common.h"

void uart_init(){
    /* Initialize UART. */
    U1RXR = 2;                          /* assign UART1 receive to pin RPF4 */
    RPF5R = 1;                          /* assign pin RPF5 to UART1 transmit */

    U1BRG = PIC32_BRG_BAUD (MHZ * 500000, 115200);
    U1STA = 0;
    U1MODE = PIC32_UMODE_PDSEL_8NPAR |      /* 8-bit data, no parity */
             PIC32_UMODE_ON;                /* UART Enable */
    U1STASET = PIC32_USTA_URXEN |           /* Receiver Enable */
               PIC32_USTA_UTXEN;            /* Transmit Enable */

}

int uart_putc(int c){
    /* Wait for transmitter shift register empty. */
    while (! (U1STA & PIC32_USTA_TRMT))
        continue;
again:
    /* Send byte. */
    U1TXREG = c;

    /* Wait for transmitter shift register empty. */
    while (! (U1STA & PIC32_USTA_TRMT))
        continue;

    if (c == '\n') {
        c = '\r';
        goto again;
    }
    return c;
}

int uart_puts(const char* str){
    int n = 0;
    while(*str){
        uart_putc(*str++);
        n++;
    }
    uart_putc('\n');
    return n + 1;
}

int uart_write(const char* str, size_t n){
    while(n--) uart_putc(*str++);
    return n;
}

unsigned char uart_getch(){
      unsigned c;

    for (;;) {
        /* Wait until receive data available. */
        if (U1STA & PIC32_USTA_URXDA) {
            c = (unsigned char) U1RXREG;
            break;
        }
    }
    return c;
}
