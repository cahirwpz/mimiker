# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

SOURCES_C = atkbdc.c \
	    rtc.c \
	    pit.c \
	    stdvga.c


all:	$(DEPFILES) libdrv.a

include $(TOPDIR)/build/build.kern.mk

libdrv.a: $(OBJECTS)

clean:
	$(RM) .*.D *.ko *.o
	$(RM) tags *.taghl
	$(RM) libdrv.a
