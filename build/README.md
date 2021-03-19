Mimiker Build System
======================

###Command line options

The following command line options are provided:
- BOARD: The target board. Defaults to `malta`.
- VERBOSE: 1-recips are loud. Otherwise, recips are quiet.
- KASAN: 1-employ the kernel address sanitizer, otherwise don't. Defaults to 0.
- LOCKDEP: 1-employ the lock dependency validator, otherwise don't. Defaults to 0.
- CLANG: 1-use Clang, otherwise use GCC.

###API between makefiles

Each makefile available in the build directory may be included by other makefiles. In such a case, a set of input make variables must be provided by the including makefile. The set of required make variables along with a brief description are supplied at the beginning of each makefile.
