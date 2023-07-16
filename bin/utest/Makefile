TOPDIR = $(realpath ../..)

SOURCES = \
	access.c \
	cred.c \
	exceptions.c \
	fd.c \
	fork.c \
	fpu_ctx.c \
	getcwd.c \
	lseek.c \
	main.c \
	misbehave.c \
	mmap.c \
	mprotect.c \
	pgrp.c \
	pipe.c \
	procstat.c \
	pty.c \
	sbrk.c \
	signal.c \
	stat.c \
	setjmp.c \
	sigaction.c \
	time.c \
	tty.c \
	utest.c \
	utest.h \
	util.c \
	util.h \
	vfs.c \
	wait.c \
	vm_map.c \
	vm_map_prot.c \

LIBWRAP = \
	xaccess.c \
	xchdir.c \
	xchmod.c \
	xfork.c \
	xfstat.c \
	xgetgroups.c \
	xgetresgid.c \
	xgetresuid.c \
	xgettimeofday.c \
	xmkdir.c \
	xmprotect.c \
	xmunmap.c \
	xkill.c \
	xkillpg.c \
	xlchmod.c \
	xlink.c \
	xlstat.c \
	xpipe.c \
	xrmdir.c \
	xsetgroups.c \
	xsetresgid.c \
	xsetresuid.c \
	xsignal.c \
	xstat.c \
	xsymlink.c \
	xunlink.c

PROGRAM = utest

SOURCES += $(addprefix libwrap/,$(LIBWRAP))
CPPFLAGS += -I.
EXTRAFILES = $(shell find extra -type f)
INSTALL-FILES = $(EXTRAFILES:extra/%=$(SYSROOT)/%)

include $(TOPDIR)/build/build.prog.mk

# Force a user not to use following symbols
LDFLAGS += --wrap=sprintf --wrap=printf

$(SYSROOT)/%: extra/%
	@echo "[INSTALL] $(DIR)$< -> /$*"
	$(INSTALL) -m 644 $< $@
