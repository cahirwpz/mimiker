#include "pic32mz.h"
#include "uart_raw.h"
#include "global_config.h"

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

void uart_putch(unsigned char c){
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

}

void uart_putstr(const char* str){
  while(*str) uart_putch(*str++);
}

void uart_putnstr(unsigned int n, const char* str){
    while(n--) uart_putch(*str++);
}

/* Helper function for displaying hex values/ */
static int hexchar (unsigned val)
{
    return "0123456789abcdef" [val & 0xf];
}


void uart_puthex(unsigned value){
    uart_putch (hexchar (value >> 28));
    uart_putch (hexchar (value >> 24));
    uart_putch (hexchar (value >> 20));
    uart_putch (hexchar (value >> 16));
    uart_putch (hexchar (value >> 12));
    uart_putch (hexchar (value >> 8));
    uart_putch (hexchar (value >> 4));
    uart_putch (hexchar (value));
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


void uart_printreg(const char* p, unsigned value){
    uart_putstr (p);
    uart_putch ('=');
    uart_putch (' ');
    uart_puthex (value);
    uart_putch ('\n');
}
