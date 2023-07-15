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
	xfork.c \
	xkill.c \
	xsignal.c

PROGRAM = utest

EXTRAFILES = $(shell find extra -type f)
INSTALL-FILES = $(EXTRAFILES:extra/%=$(SYSROOT)/%)

include $(TOPDIR)/build/build.prog.mk

# Force a user not to use following symbols
LDFLAGS += --wrap=sprintf --wrap=printf

# Provide libc function wrappers that call die() on non-zero errno
LDFLAGS += --wrap=fork --wrap=kill --wrap=signal

$(SYSROOT)/%: extra/%
	@echo "[INSTALL] $(DIR)$< -> /$*"
	$(INSTALL) -m 644 $< $@
