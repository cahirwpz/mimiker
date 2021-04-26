# Onboarding note

## Project overview

Mimiker is an experiment with implementation of very simple operating
system for Malta board and Raspberry Pi 3.

Main project ideas:

- Main one is to implement only Kernel Space part of system on our own.
Keeping interface with User Space as compatible as possible with
NetBSD. That way we'll be capable of running all programs for that
system on ours (we already can run a few of those). Consequece of that
is that all header files are the same as NetBSD's.
- Target architectures are MIPS64 and AArch64.

### Project file hierarchy

Most of incoherences in file hierarchy come from expectations of C
programs to find header files in specific locations.
More information can be found in [`hier(7)`](https://man.netbsd.org/hier.7).

- `bin/` and `usr/bin/` contain programs from NetBSD that are
considered to be ready to run, because are build during compilation
  - `bin/utest/` contains our tests supposed to be run in User Space
- `contrib/` contains programs possible to be build inside running system
- `build/` elements of build system, parts shared by all Makefiles
- `etc/` config files installed in `/sysroot`
- `include/` Folder with all of header files for programs ran in User and Kernel
Space. Goes straight to `sysroot/`. That one has most of incoherences, because
everyting here is compatible with NetBSD and standard C library.
  - `include/dev/` describes hardware registers and
struct definitions. Vast majority of contained files
has been aquired. Everything that concerns drivers is
    located here.
  - `include/mips/` and `/include/aarch64` are parts
depending on architecture, partly aquired, available for Kernel
  - `include/sys/` contains hearders mostly for Kernel,
but with parts dedicated for User Space
- `lib/` contains User Space libraries ported from NetBSD, but only those we need
  - `lib\csu\` contains files `crt0` ("C runtime 0")
describing program life before `main` function a nd
after calling `return` at the end
  - `\lib\libc` contains C library files, parts that
depend on architecture are written in Assembly
- `sys/` contains Kernel files
  - `sys/kern/` contains main part of Kernel
  - `sys/debug` Python scripts extending GDB capabilities.
  - `sys/drv` system drivers
  - `sys/script` scripts generating code?
  - `sys/tests` tests ran in Kernel Space
  - `sys/.../` are parts hardware dependent
- `toolchain/` tools for developer environment
  - `toolchain/qemu-mimiker/` port of QEMU - machine
  emulator and virtualizer for running our system
  - `toolchain/openocd-mimiker` port of OpenOCD -
  interface for hardware debugger. Adapter between Raspberry Pi 3 board and GDB
- `Dockerfile` description of environment for GitHub Actions build and testing
- `.github/workflows/` files for Continuous integration
- `wiki/`

### Licence

This project is licensed under the BSD 3-Clause. Details of code we can
use can be found [here](https://en.wikipedia.org/wiki/License_compatibility) and [this](https://en.wikipedia.org/wiki/License_compatibility#/media/File:Floss-license-slide-image.svg) is a nice cheatsheet.
Simplest inferences include facts that we can use all the code under MIT
licence and we are not supposed to look at or be inspired by any code
under GPL.
