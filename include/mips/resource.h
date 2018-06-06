#ifndef _MACHINE_RESOURCE_H_
#define _MACHINE_RESOURCE_H_    1

#define SYS_RES_PCI_IO 1
#define SYS_RES_PCI_MEM 2
#define SYS_RES_ISA 4

/* We need PCI-ISA bridge in order to define resource types like this: */
#if 0
#define SYS_RES_IRQ 1
#define SYS_RES_DRQ 2
#define SYS_RES_MEMORY 3
#define SYS_RES_IOPORT 4
#endif

#endif