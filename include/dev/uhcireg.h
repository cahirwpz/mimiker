/*
 * Based on FreeBSD `uhcireg.h`.
 */
#ifndef _UHCIREG_H_
#define _UHCIREG_H_

/* PCI config registers  */
#define PCI_USB_CLASSCODE 0x0c
#define PCI_USB_SUBCLASSCODE 0x03
#define PCI_USBREV 0x60 /* USB protocol revision */
#define PCI_USB_REV_PRE_1_0 0x00
#define PCI_USB_REV_1_0 0x10
#define PCI_USB_REV_1_1 0x11
#define PCI_INTERFACE_UHCI 0x00

/* UHCI registers */
#define UHCI_CMD 0x00
#define UHCI_CMD_RS 0x0001
#define UHCI_CMD_HCRESET 0x0002
#define UHCI_CMD_GRESET 0x0004
#define UHCI_CMD_MAXP 0x0080
#define UHCI_STS 0x02
#define UHCI_STS_USBINT 0x0001
#define UHCI_STS_USBEI 0x0002
#define UHCI_STS_HCH 0x0020
#define UHCI_INTR 0x04
#define UHCI_INTR_TOCRCIE 0x0001
#define UHCI_INTR_IOCE 0x0004
#define UHCI_FRNUM 0x06
#define UHCI_FLBASEADDR 0x08
#define UHCI_SOF 0x0c
#define UHCI_PORTSC(n) (0x10 + (n)*2)
#define UHCI_PORTSC_CCS 0x0001
#define UHCI_PORTSC_CSC 0x0002
#define UHCI_PORTSC_PE 0x0004
#define UHCI_PORTSC_PEC 0x0008
#define UHCI_PORTSC_DP 0x0010
#define UHCI_PORTSC_DM 0x0020
#define UHCI_PORTSC_RD 0x0040
#define UHCI_PORTSC_ONE 0x0080
#define UHCI_PORTSC_LSDA 0x0100
#define UHCI_PORTSC_PR 0x0200
#define UHCI_PORTSC_OCI 0x0400
#define UHCI_PORTSC_OCIC 0x0800
#define UHCI_PORTSC_SUSP 0x1000

#endif /* _UHCIREG_H_ */
