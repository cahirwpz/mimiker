Toolchain
---

To build Mimiker you will need a custom toolchain we use. It contains cross
compiler (`gcc`), linker (`binutils`) and debugger (`gdb`). We prepared prebuild
binary package for Debian x86-64 based system, each for different target
architecture: [MIPS][1], [AArch64][2], RISC-V [32-bit][3] and [64-bit][4].
You need to install patched version of QEmu as well, you can find it [here][5].

[1]: http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_latest_amd64.deb
[2]: http://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_latest_amd64.deb
[3]: http://mimiker.ii.uni.wroc.pl/download/riscv32-mimiker-elf_latest_amd64.deb
[4]: http://mimiker.ii.uni.wroc.pl/download/riscv64-mimiker-elf_latest_amd64.deb
[5]: http://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_latest_amd64.deb
