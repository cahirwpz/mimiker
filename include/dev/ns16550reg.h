/* Described in http://www.ti.com/product/pc16550d */

#define RBR 0 /* Receiver Buffer, read-only, DLAB = 0 */
#define THR 0 /* Transmitter Holding, write-only, DLAB = 0 */
#define DLL 0 /* Divisor Latch LSB, read-write, DLAB = 1 */
#define IER 1 /* Interrupt Enable, read-write, DLAB = 0 */
#define DLM 1 /* Divisor Latch LSB read-write, DLAB = 1 */
#define IIR 2 /* Interrupt Identification, read-only */
#define FCR 2 /* FIFO Control, write-only */
#define LCR 3 /* Line Control, read-write */
#define MCR 4 /* Modem Control, read-write */
#define LSR 5 /* Line Status, read-only */
#define MSR 6 /* Modem Status, read-only */

#define IER_ERXRDY 0x1
#define IER_ETXRDY 0x2

#define IIR_RXTOUT 0xc
#define IIR_BUSY 0x7
#define IIR_RLS 0x6
#define IIR_RXRDY 0x4
#define IIR_TXRDY 0x2
#define IIR_NOPEND 0x1
#define IIR_MLSC 0x0

#define LCR_DLAB 0x80
#define LCR_SBREAK 0x40
#define LCR_PZERO 0x30
#define LCR_PONE 0x20
#define LCR_PEVEN 0x10
#define LCR_PENAB 0x08
#define LCR_STOPB 0x04
#define LCR_8BITS 0x03
#define LCR_7BITS 0x02
#define LCR_6BITS 0x01
#define LCR_5BITS 0x00

#define LSR_RCV_FIFO 0x80
#define LSR_TEMT 0x40
#define LSR_THRE 0x20
#define LSR_BI 0x10
#define LSR_FE 0x08
#define LSR_PE 0x04
#define LSR_OE 0x02
#define LSR_RXRDY 0x01
#define LSR_RCV_MASK 0x1f

#define MSR_DCD 0x80
#define MSR_RI 0x40
#define MSR_DSR 0x20
#define MSR_CTS 0x10
#define MSR_DDCD 0x08
#define MSR_TERI 0x04
#define MSR_DDSR 0x02
#define MSR_DCTS 0x01

#define FCR_ENABLE 0x01
#define FCR_RCV_RST 0x02
#define FCR_XMT_RST 0x04
#define FCR_DMA 0x08
#define FCR_RX_LOW 0x00
#define FCR_RX_MEDL 0x40
#define FCR_RX_MEDH 0x80
#define FCR_RX_HIGH 0xc0
