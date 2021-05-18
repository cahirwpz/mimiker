
# Onboarding note

## Project overview

Mimiker is an experimental implementation of very simple UNIX-like operating
system for Malta board and Raspberry Pi 3.

### Main project goals

* to be a research system helping us to understand structure of UNIX kernels
* implement only Kernel of the system on our own.
* keep User Space interface as compatible as possible with
  NetBSD. That way we'll be capable to run all programs designed for that
  system, we already are able to run a few of those. Consequence of that
  is that all header files are the same as NetBSD's.
* We want kernel to be run on a modern architecture, we started with MIPS32,
  but currently we are switching to AArch64 (Raspberry Pi 3)
* We love to KISS

### More significant tools we use to run system

* QEMU - virtual machine allowing us to run system
* Tmux - terminal multiplexer, that allows us to access multiple terminals in
  one window
* Korn Shell (`ksh`) - simple and only one shell that we already ported

#### How system is run?

When you turn on the system with `./launch` [script][1] system is being run in QEMU.
Multiple terminals are connected to VM. Between these you can switch with Tmux.
On one of them is klog (log with kernel messages). On the second one is GDB.
The other one there can be Korn Shell working in the system.

### Project file hierarchy

Most of the incoherence in file hierarchy come from expectations of C
programs to find header files in specific locations.
More information can be found in [`hier(7)`][2].

* `bin/` and `usr.bin/` programs from NetBSD that are
  considered to be ready to run, because are build during compilation
  * `bin/utest/` tests, that are supposed to be run in User Space,
    testing mostly syscalls
* `contrib/` programs we are capable to build without significant modifications.
  Modifications, if any, are applied as patches
* `build/` build scripts, shared by all Makefiles
* `etc/` system configuration files and scripts
* `include/` folder with all the header files for code ran in User and Kernel
  Space. That one has most of the incoherences, because
  everything here is compatible with NetBSD and standard C library.
  * `include/dev/` description of hardware registers and struct definitions.
  Vast majority of contained files has been acquired from existing systems.
  Everything that concerns drivers are located here.
  * `include/mips/` and `/include/aarch64` parts depending on architecture,
    partly acquired, available for Kernel
  * `include/sys/` headers mostly for Kernel, but with parts dedicated for
    User Space
* `lib/` User Space libraries ported from NetBSD, but only these we actually need
  * `lib/csu/` files `crt0` ("C runtime 0") describing program life before
  `main` function and after calling `return` at the end
  * `lib/libc/` C library files, parts that depend on architecture are written
    in Assembly
* `sys/` actual Kernel code
  * `sys/kern/` main part of Kernel
  * `sys/debug` Python scripts extending GDB capabilities.
  * `sys/drv` system drivers for hardware devices
  * `sys/script` scripts generating glue code, i.e. syscalls
  * `sys/tests` tests ran in Kernel Space
  * `sys/aarch64/` and `sys/mips/` hardware dependent parts
* `toolchain/` tools for development environment
  * `toolchain/gnu/` mimiker compiler toolchain containing
    cross-compiler for building mimiker on your desktop.
    It comes in two versions for both supported platforms
  * `toolchain/qemu-mimiker/` debian install scripts and patches for QEMU -
    machine emulator and simulator for running mimiker
  * `toolchain/openocd-mimiker` port of OpenOCD -
    Interface for hardware debugger. Adapter between Raspberry Pi 3 board and GDB
* `Dockerfile` description of environment for GitHub Actions build and testing
* `.github/workflows/` YAML scripts for Continuous integration
  * `.github/workflows/default.yml` CI script that builds system and
    tests it in cloud. Run every time new commit it pushed on any branch.
  * `.github/workflows/deploy_wiki.yml` GitHub Actions script that runs
    script from `wiki/` every time new wiki document file lands on Master
* `wiki/` contains documents of our wiki and script
  that publishes files from `wiki/docs` to GitHub Wiki

### Licence

This project is licensed under the BSD 3-Clause. Details of code we can
use can be found [here][3] and [that][4] is a nice cheat sheet.
The Simplest inferences include facts that we can use all the code under MIT
licence and we are not supposed to look at or be inspired by any code
under GPL.

[1]: https://github.com/cahirwpz/mimiker#readme
[2]: https://man.netbsd.org/hier.7
[3]: https://en.wikipedia.org/wiki/License_compatibility
[4]: https://en.wikipedia.org/wiki/License_compatibility#/media/File:Floss-license-slide-image.svg

