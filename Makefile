# vim: tabstop=8 shiftwidth=8 noexpandtab:

TESTS = \
	callout.elf \
	exec.elf \
	exec_syscall.elf \
	malloc.elf \
	mutex.elf \
	physmem.elf \
	pmap.elf \
	rtc.elf \
	sched.elf \
	sleepq.elf \
	syscall.elf \
	thread.elf \
	vm_map.elf \
	argv_test.elf
SOURCES_C =
SOURCES_ASM =

all: tags cscope $(TESTS)

include Makefile.common
$(info Using CC: $(CC))

# Directories which require calling make recursively
SUBDIRS = mips stdc sys user

LDLIBS += -Lsys -Lmips -Lstdc \
	  -Wl,--start-group -lsys -lmips -lstdc -lgcc -Wl,--end-group
# Files that need to be embedded alongside kernel image
LD_EMBED = user/prog.uelf.o user/syscall_test.uelf.o

# Files required to link kernel image
KRT = stdc/libstdc.a mips/libmips.a sys/libsys.a $(LD_EMBED)

cscope:
	cscope -b include/*.h ./*.[cS]

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


define emit_subdir_rules
# To make a directory, call make recursively
$(1):
	$$(MAKE) -C $$@
endef
define emit_krt_rule
# To make an embeddable file, you need to make its directory
$(1): $(patsubst %/,%,$(dir $(1)))
	# This target has to override the generic %.a rule from Makefile.common, so
	# that it won't get invoked in this case. Otherwise we would try to build,
	# say, "sys/libsys.a" from "sys" with ar.
	true
endef
define emit_test_rule
# To build a test .elf, you require the corresponding .ko and all of $(KRT)
$(1): $(1:%.elf=%.ko) $(KRT)
endef

# Generate targets according to rules above
$(foreach subdir, $(SUBDIRS), $(eval $(call emit_subdir_rules,$(subdir))))
$(foreach file, $(KRT), $(eval $(call emit_krt_rule,$(file))))
$(foreach test, $(TESTS), $(eval $(call emit_test_rule,$(test))))


test:
	for file in $(wildcard *.test); do		\
	  echo -n "Running $${file}... ";		\
	  if ./$${file}; then				\
	    echo "\033[32;1mPASSED\033[0m";		\
	  else						\
	    echo "\033[31;1mFAILED\033[0m";		\
	  fi						\
	done

clean:
	$(foreach DIR, $(SUBDIRS), $(MAKE) -C $(DIR) $@;)
	$(RM) -f .*.D *.ko *.o *.a *.lst *~ *.elf *.map *.log
	$(RM) -f tags cscope.out *.taghl
	$(RM) -f $(TESTS)

.PHONY: format tags cscope mips stdc sys user
