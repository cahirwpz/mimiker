# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = \
	dev_cons.c \
	uart.c \
	usb.c

SOURCES-MIPS = \
	82371AB.c \
	atkbdc.c \
	cbus.c \
	evdev.c \
	evdev_kbd.c \
	gt64120.c \
	isa.c \
	malta_rootdev.c \
	ns16550.c \
	rtc.c \
	rtl8139.c \
	pci.c \
	pci_ids.c \
	pit.c \
	stdvga.c \
	uart_cbus.c \
	uhci.c

SOURCES-AARCH64 = \
	bcm2835_gpio.c \
	bcm2835_rootdev.c \
	pl011.c

SOURCES-RISCV = \
	clint.c \
	liteuart.c \
	plic.c \
	riscv_rootdev.c

CPPFLAGS += -D_MACHDEP

include $(TOPDIR)/build/build.kern.mk
