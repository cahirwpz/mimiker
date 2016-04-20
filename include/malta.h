#ifndef __MALTA_H__
#define __MALTA_H__

/*
 * Malta Memory Map:
 *
 *  0000.0000 +     128MB   Typically SDRAM (on Core Board)
 *  0800.0000 +     256MB   Typically PCI
 *  1800.0000 +      62MB   Typically PCI
 *  1be0.0000 +       2MB   Typically System controller's internal registers
 *  1c00.0000 +      32MB   Typically not used
 *  1e00.0000         4MB   Monitor Flash
 *  1e40.0000        12MB   reserved
 *  1f00.0000        12MB   Switches
 *                          LEDs
 *                          ASCII display
 *                          Soft reset
 *                          FPGA revision number
 *                          CBUS UART (tty2)
 *                          General Purpose I/O
 *                          I2C controller
 *  1f10.0000 +      11MB   Typically System Controller specific
 *  1fc0.0000         4MB   Maps to Monitor Flash
 *  1fd0.0000 +       3MB   Typically System Controller specific
 *
 *  + depends on implementation of the Core Board and of software
 *
 * CPU interrupts:
 *
 *  NMI     South Bridge or NMI button
 *    0     South Bridge INTR
 *    1     South Bridge SMI
 *    2     CBUS UART (tty2)
 *    3     COREHI (Core Card)
 *    4     CORELO (Core Card)
 *    5     Not used, driven inactive (typically CPU internal timer interrupt
 *
 * IRQ mapping (as used by YAMON):
 *
 *    0      Timer           South Bridge
 *    1      Keyboard        SuperIO
 *    2                      Reserved by South Bridge (for cascading)
 *    3      UART (tty1)     SuperIO
 *    4      UART (tty0)     SuperIO
 *    5                      Not used
 *    6      Floppy Disk     SuperIO
 *    7      Parallel Port   SuperIO
 *    8      Real Time Clock South Bridge
 *    9      I2C bus         South Bridge
 *   10      PCI A,B,eth     PCI slot 1..4, Ethernet
 *   11      PCI C,audio     PCI slot 1..4, Audio, USB (South Bridge)
 *           PCI D,USB
 *   12      Mouse           SuperIO
 *   13                      Reserved by South Bridge
 *   14      Primary IDE     Primary IDE slot
 *   15      Secondary IDE   Secondary IDE slot/Compact flash connector
 */

#define MALTA_CORECTRL_BASE     0x1be00000
#define MALTA_FPGA_BASE         0x1f000000

#define MALTA_CBUS_UART         (MALTA_FPGA_BASE + 0x900)
#define MALTA_CBUS_UART_INTR    2

#endif
