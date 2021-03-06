TARGETS = mipsel aarch64

all: package

build-lib:
	$(MAKE) -C lib

clean-lib:
	$(MAKE) -C lib clean

# Generate rules for given target
define emit_target_rules
build-$(1): $(1)-mimiker-elf build-lib
	$(MAKE) -C $(1)-mimiker-elf

$(1)-mimiker-elf_$(VERSION)_amd64.deb: build-$(1)
	$(MAKE) -C $(1)-mimiker-elf package

package-$(1): $(1)-mimiker-elf_$(VERSION)_amd64.deb

clean-$(1):
	$(MAKE) -C $(1)-mimiker-elf clean
	$(RM) $(1)-*.deb

PHONY-TARGETS += build-$(1) clean-$(1)
endef

$(foreach dir,$(TARGETS),$(eval $(call emit_target_rules,$(dir))))

build: $(TARGETS:%=build-%)

package: $(TARGETS:%=package-%)

clean: $(TARGETS:%=clean-%) clean-lib

.PHONY: all build clean build-lib clean-lib $(PHONY-TARGETS)

# vim: ts=8 sw=8 noet
