# vim: tabstop=8 shiftwidth=8 noexpandtab:

SUBDIR-install := $(SUBDIR:%=%-install)
SUBDIR-clean := $(SUBDIR:%=%-clean)

$(SUBDIR):
	$(MAKE) -C $@

install: $(SUBDIR-install)
clean: $(SUBDIR-clean)

define emit_subdir_rule
$(1)-$(2):
	$(MAKE) -C $(1) $(2)
endef

$(foreach dir,$(SUBDIR) null,$(eval $(call emit_subdir_rule,$(dir),install)))
$(foreach dir,$(SUBDIR) null,$(eval $(call emit_subdir_rule,$(dir),clean)))

.PHONY: $(SUBDIR) $(SUBDIR-install) $(SUBDIR-clean)

include $(TOPDIR)/build/common.mk
