TARGETS = mipsel

TOOLCHAIN = ./toolchain-mipsel
RM = rm -v -f

all: $(TARGETS)

mipsel:
	$(TOOLCHAIN) build
	cd $@-mimiker-elf && fakeroot ./debian/rules binary

clean:
	$(TOOLCHAIN) clean
	$(RM) *.deb
	for target in $(TARGETS); do \
	  cd $${target}-mimiker-elf && \
	    $(RM) -r sources usr debian/$${target}-mimiker-elf.*; \
	done
	$(RM) *~
	

.PHONY: all $(TARGETS) clean

# vim: ts=8 sw=8 noet
