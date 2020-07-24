#ifndef _MIPS_MALTA_H_
#define _MIPS_MALTA_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <sys/cdefs.h>

/*! \brief Prepares thread0 stack that is placed in kseg2. */
void *board_stack(int argc, char **argv, char **envp, unsigned memsize);

/*! \brief Executes platform initialization code placed in kseg2. */
__noreturn void board_init(void);

/*
 * Malta Memory Map:
 *
 *  0000.0000   128MB   Basic SDRAM
 *  0800.0000   128MB   Extra SDRAM
 *  1000.0000   128MB   PCI memory space
 *  1800.0000    62MB   PCI I/O space
 *  1be0.0000     2MB   System Controller's internal registers
 *  1e00.0000     4MB   Monitor Flash
 *  1f00.0000    12MB   Switches
 *                      LEDs
 *                      ASCII display
 *                      Soft reset
 *                      FPGA revision number
 *                      CBUS UART (tty2)
 *                      General Purpose I/O
 *                      I2C controller
 *  1f10.0000    11MB   System Controller specific
 *  1fc0.0000     4MB   Monitor Flash alias
 *  1fd0.0000     3MB   System Controller specific
 *
 * CPU interrupts:
 *
 *  NMI     PIIX4 NMI or NMI button
 *    0     PIIX4 Interrupt Request
 *    1     PIIX4 System Management Interrupt
 *    2     CBUS UART2
 *    3     COREHI (Core Card)
 *    4     CORELO (Core Card)
 *    5     Internal CPU timer interrupt
 *
 * http://wiki.osdev.org/8259_PIC
 *
 * PIIX4 master PIC 8259A:
 *
 *    0      System Timer      PIIX4
 *    1      Keyboard          Super I/O
 *    2      –                 Reserved by PIIX4 (for cascading)
 *    3      UART1             Super I/O
 *    4      UART0             Super I/O
 *    5      –                 Not used
 *    6      Floppy Disk       Super I/O
 *    7      Parallel Port     Super I/O
 *
 * PIIX4 slave PIC 8259A:
 *
 *    8      Real Time Clock   PIIX4
 *    9      I²C bus           PIIX4
 *   10      PCI INTA#,INTB#   Ethernet controller
 *   11      PCI INTC#,INTD#   USB controller (PIIX4)
 *   12      Mouse             Super I/O
 *   13      –                 Reserved by PIIX4
 *   14      Primary IDE       PIIX4
 *   15      Secondary IDE     PIIX4
 */

#define MALTA_PHYS_SDRAM_BASE 0x00000000

#define MALTA_PCI0_MEMORY_BASE 0x10000000
#define MALTA_PCI0_MEMORY_END 0x17ffffff
#define MALTA_PCI0_IO_BASE 0x18000000
#define MALTA_PCI0_IO_END 0x1bdfffff
#define MALTA_CORECTRL_BASE 0x1be00000
#define MALTA_CORECTRL_END 0x1dffffff
#define MALTA_FPGA_BASE 0x1f000000

#define MALTA_CBUS_UART_OFFSET 0x900
#define MALTA_CBUS_UART_INTR 2

#endif
