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
	pgrp.c \
	sbrk.c \
	signal.c \
	stat.c \
	setjmp.c \
	sigaction.c \
	utest.c \
	vfs.c \
	wait.c

PROGRAM = utest

EXTRAFILES = $(shell find extra -type f)
INSTALL-FILES = $(EXTRAFILES:extra/%=$(SYSROOT)/%)

include $(TOPDIR)/build/build.prog.mk

$(SYSROOT)/%: extra/%
	@echo "[INSTALL] $(DIR)$< -> /$*"
	$(INSTALL) -m 644 $< $@
