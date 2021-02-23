RM = rm -v -f
CP = cp -v

TARGETS = mipsel

all: build

# Generate rules for given target
define emit_target_rules
build-$(1): $(1)-mimiker-elf
	$(MAKE) -C $(1)-mimiker-elf

clean-$(1):
	$(MAKE) -C $(1)-mimiker-elf clean

PHONY-TARGETS += build-$(1) clean-$(1)
endef

$(foreach dir,$(TARGETS),$(eval $(call emit_target_rules,$(dir))))

build: $(TARGETS:%=build-%)

clean: $(TARGETS:%=clean-%)

.PHONY: all build clean $(PHONY-TARGETS)

# vim: ts=8 sw=8 noet
