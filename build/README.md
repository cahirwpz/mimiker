Mimiker Build System
======================

### How does it work?

- The following standard targets are defined: download, install, build, 
  clean, distclean, and format.
- To satisfy any of the standard targets, the analogous target in all
  the directories listed in SUBDIR must be satisfied first 
  (using depth-first traversal). The only exception from this rule is the
  format target, which will apply the recursion only if FORMAT-RECURSE
  is undefined.
- For each directory listed in SUBDIR, a special `subdir`-before target
  may be supplied. If such a target exists, it will be executed before
  executing the build target of the subdirectory.
- The including makefile can define a `standard target`-here target which
  will be executed after satisfying `standard target` for each of the
  subdirectories.
- Through the makefiles the TOPDIR variable is used. Whenever it occurs,
  the including makefile should set it to the path to the mimiker directory
  in the host system.
- All make variables composing an API of a makefile from the build directory
  must be set by the including makefile before the makefile is included.

### Interfaces between makefiles

Each makefile available in the build directory may be included by other
makefiles. A makefile defines a set of make variables which are to be set by the
including makefile. Those variables along with a brief description are supplied
at the beginning of each makefile. If a variable is said to have a default value
or other variables condition its usage, then such a variable is optional.
Otherwise, the variable must be supplied in the including makefile and must be
set before including the makefile. Throughout the makefiles composing the build
system, the special make variable TOPDIR is used. It should always be set by the
top-level makefile to the path to the mimiker directory in the host system.

### Command line variables

The following variables can be given at `make` command line:
- BOARD: The target board. Currently supported boards: `malta`, `rpi3`.
  Defaults to `malta`.
- VERBOSE: 1-recipes are loud. Otherwise, recipes are quiet.
- KASAN: 1-employ the kernel address sanitizer, otherwise don't.
  Defaults to 0.
- LOCKDEP: 1-employ the lock dependency validator, otherwise don't.
  Defaults to 0.
- CLANG: 1-use Clang, otherwise use GCC.

### Common variables

The following make variables are set by the including makefile:
- SOURCES: Source files to compile (C, assembler, Yacc, ...)
- BINDIR: The path relative to $(SYSROOT) at which program will be installed.
- BINMODE: Permissions with which main file will be installed. 
- BUILD-FILES: Files to build at the current recursion level.
- INSTALL-FILES: Files to install at the current recursion level.
- CLEAN-FILES: Files to remove at the current recursion level.
- FORMAT-EXCLUDE: Files that shouldn't be formatted.
- FORMAT-RECURSE: Should be set to "no" if recursion shouldn't be applied
  to SUBDIR. Otherwise, must be undefined.
- PHONY-TARGETS: Phony targets.
- DEPENDENCY-FILES: Files specifying dependencies (e.g. `*.D` files).
