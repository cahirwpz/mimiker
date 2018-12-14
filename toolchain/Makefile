TOOLCHAIN = ./toolchain
RM = rm -v -f
CP = cp -v

TARGETS = mipsel aarch64

all:

%-mimiker-elf:
	$(CP) -a toolchain-mimiker-elf $@
	sed -i -e 's#%{TARGET}#$(@:%-mimiker-elf=%)#g' \
	       -e 's#%{DATE}#$(shell date -R)#g' \
	       `find $@ -type f`

# Generate rules for given target
define emit_target_rules
build-$(1): $(1)-mimiker-elf
	$(TOOLCHAIN) --target=$(1) build
	cd $(1)-mimiker-elf && fakeroot ./debian/rules binary

clean-$(1):
	$(RM) -r $(1)-mimiker-elf
	$(RM) $(1)-*.deb

PHONY-TARGETS += build-$(1) clean-$(1)
endef

$(foreach dir,$(TARGETS),$(eval $(call emit_target_rules,$(dir))))

build: $(TARGETS:%=build-%)

clean: $(TARGETS:%=clean-%)
	$(RM) *~

.PHONY: all build clean $(PHONY-TARGETS)

# vim: ts=8 sw=8 noet
