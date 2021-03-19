Mimiker Build System
======================

### Command line options

The following command line options are provided:
- BOARD: The target board. Defaults to `malta`.
- VERBOSE: 1-recipes are loud. Otherwise, recipes are quiet.
- KASAN: 1-employ the kernel address sanitizer, otherwise don't. Defaults to 0.
- LOCKDEP: 1-employ the lock dependency validator, otherwise don't. Defaults to 0.
- CLANG: 1-use Clang, otherwise use GCC.

### API between makefiles

Each makefile available in the build directory may be included by other makefiles. A makefile defines a set of make variables which are to be set by the including makefile. Those variables along with a brief description are supplied at the beginning of each makefile. If a variable is said to have a default value or other variables condition its usage, then such a variable is optional. Otherwise, the variable must be supplied be the including makefile and must be set before including the makefile. Throughout the makefiles composing the build system, the special make variable TOPDIR is used. It should always be set by the top-level makefile to the path to the mimiker directory in the host system.
