TOPDIR = $(realpath ../..)

NEW_SRC = font.c input.c
SRC = st.c x.c
HDR = arg.h st.h win.h

PROGRAM = st
LDLIBS = -lutil
BINDIR = bin
BUILDDIR = build/
SOURCES = $(addprefix st/,$(SRC)) $(addprefix $(BUILDDIR)st/,$(NEW_SRC))
FORMAT-EXCLUDE = $(addprefix st/,$(SRC)) $(addprefix st/,$(HDR))

include $(TOPDIR)/build/build.prog.mk

# Rules for newly created source files:

define new_src_build =
$$(addprefix $(BUILDDIR)st/,$(1)): build-before
endef

$(foreach new_src,$(NEW_SRC),$(eval $(call new_src_build,$(new_src))))

define new_src_include =
CFLAGS.$(BUILDDIR)st/$(1) += -Ist/
endef

$(foreach new_src,$(NEW_SRC),$(eval $(call new_src_include,$(new_src))))

# Main rules for downloading st and patching:

download-here: st/.git

build-before: quilt-patch

clean-here: quilt-unpatch

# Rules for generating config.h:

$(OBJECTS): $(BUILDDIR)st/config.h

$(BUILDDIR)%.h: %.def.h
	cp $< $@

CPPFLAGS += -I$(BUILDDIR)/st
