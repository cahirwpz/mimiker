# vim: tabstop=8 shiftwidth=8 noexpandtab:

all: tags cscope tests

include Makefile.common
$(info Using CC: $(CC))

# Directories which require calling make recursively
SUBDIRS = mips stdc sys user tests

# This ensures that the test subdirectory may only be built once these specified
# dirs are brought up-to-date.
tests: | mips stdc sys user

cscope:
	cscope -b include/*.h ./*/*.[cS]

tags:
	find -iname '*.[ch]' -not -path "*/toolchain/*" | ctags --language-force=c -L-
	find -iname '*.[ch]' -not -path "*/toolchain/*" | ctags --language-force=c -L- -e -f etags
	find -iname '*.S' -not -path "*/toolchain/*" | ctags -a --language-force=asm -L-
	find -iname '*.S' -not -path "*/toolchain/*" | ctags -a --language-force=asm -L- -e -f etags

# These files get destroyed by clang-format, so we exclude them from formatting
FORMATTABLE_EXCLUDE = include/elf stdc/smallclib include/mips/asm.h include/mips/m32c0.h
# Search for all .c and .h files, excluding toolchain build directory and files from FORMATTABLE_EXCLUDE
FORMATTABLE = $(shell find -type f -not -path "*/toolchain/*" -and \( -name '*.c' -or -name '*.h' \) | grep -v $(FORMATTABLE_EXCLUDE:%=-e %))
format:
	@echo "Formatting files: $(FORMATTABLE:./%=%)"
	clang-format -style=file -i $(FORMATTABLE)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	$(foreach DIR, $(SUBDIRS), $(MAKE) -C $(DIR) $@;)
	$(RM) -f *.a *.lst *~ *.log
	$(RM) -f tags etags cscope.out *.taghl

.PHONY: format tags cscope $(SUBDIRS)
