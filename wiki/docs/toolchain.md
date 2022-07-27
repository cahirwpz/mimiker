Toolchain
---

To build Mimiker you will need a toolchain, i.e. compiler, linker, ELF tools and
debugger. The default option is to choose quite recent _LLVM toolchain_ (check
version in [tools.mk][6]), i.e. `clang`, `lld`, `llvm` from [apt.llvm.org][7],
and [gdb-multiarch][8]. However you'll need to reconfigure [launch][9] script to
use same `gdb-multiarch` binary for each target architecture, look for `gdb`
section and find `binary` key. 

You need to install patched version of [QEmu][11] as well. Our version solves
several issues not patched in the mainstream version – please refer to our list
of [patches][10]. Prebuild package for Debian x86-64 can be found [here][5].

The other (deprecated) method is to use a custom pre-build _GNU toolchain_,
i.e. `gcc`, `binutils` and `gdb`. We prepared packages for Debian x86-64 based
system, each for different target architecture: supports [MIPS][1],
[AArch64][2], RISC-V [32-bit][3] and [64-bit][4].

[1]: http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_latest_amd64.deb
[2]: http://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_latest_amd64.deb
[3]: http://mimiker.ii.uni.wroc.pl/download/riscv32-mimiker-elf_latest_amd64.deb
[4]: http://mimiker.ii.uni.wroc.pl/download/riscv64-mimiker-elf_latest_amd64.deb
[5]: http://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_latest_amd64.deb
[6]: build/tools.mk
[7]: https://apt.llvm.org/
[8]: https://packages.debian.org/sid/gdb-multiarch
[9]: launch#L50
[10]: toolchain/qemu-mimiker/patches
[11]: https://www.qemu.org/
