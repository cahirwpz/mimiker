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

/* Helper function for displaying hex values/ */
static int hexchar (unsigned val)
{
    return "0123456789abcdef" [val & 0xf];
}


void uart_puthex(unsigned value){
    uart_putc (hexchar (value >> 28));
    uart_putc (hexchar (value >> 24));
    uart_putc (hexchar (value >> 20));
    uart_putc (hexchar (value >> 16));
    uart_putc (hexchar (value >> 12));
    uart_putc (hexchar (value >> 8));
    uart_putc (hexchar (value >> 4));
    uart_putc (hexchar (value));
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
    uart_puts (p);
    uart_putc ('=');
    uart_putc (' ');
    uart_puthex (value);
    uart_putc ('\n');
}
