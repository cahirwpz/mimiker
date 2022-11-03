Toolchain
---

To build and run Mimiker you will need a toolchain, i.e. compiler, linker, ELF
tools, emulator and debugger. The default option is to choose quite recent
_LLVM toolchain_ (check version in [tools.mk][6]), i.e. `clang`, `lld`, `llvm`
from [apt.llvm.org][7], [QEmu][11] and [gdb-multiarch][8].

NOTE:
If you plan to run Mimiker on MIPS you need to install patched version of QEmu
as well. Our version solves several issues not patched in the mainstream version
– please refer to our list of [patches][10]. Prebuild package for Debian x86-64
can be found [here][5].

NOTE:
To use gdb-multiarch you need to reconfigure [launch][9] script to use same
`gdb-multiarch` binary for each target architecture, look for `gdb` section and
find `binary` key.

## Requiremnts
You can find all needed software in a [Dockerfile][12]. There is also a script
[install-tools.sh][13] that will automatically install all needed software on
Debian system.

You also need to install python requirements. E.g. with command:
```
pip3 install -r requirements.txt
```

#### A comment about dependencies from Dockerfile
```
# patch & quilt required by lua and programs in contrib/
# gperf required by libterminfo
# socat & tmux required by launch
```

## Deprecated toolchain
The other method is to use a custom pre-build _GNU toolchain_, i.e. `gcc`,
`binutils` and `gdb`. We prepared packages for Debian x86-64 based system, each
for different target architecture: supports [MIPS][1], [AArch64][2],
RISC-V [32-bit][3] and [64-bit][4].

[1]: http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_latest_amd64.deb
[2]: http://mimiker.ii.uni.wroc.pl/download/aarch64-mimiker-elf_latest_amd64.deb
[3]: http://mimiker.ii.uni.wroc.pl/download/riscv32-mimiker-elf_latest_amd64.deb
[4]: http://mimiker.ii.uni.wroc.pl/download/riscv64-mimiker-elf_latest_amd64.deb
[5]: http://mimiker.ii.uni.wroc.pl/download/qemu-mimiker_latest_amd64.deb
[6]: ../blob/master/build/tools.mk
[7]: https://apt.llvm.org/
[8]: https://packages.debian.org/sid/gdb-multiarch
[9]: ../blob/master/launch
[10]: ../blob/master/toolchain/qemu-mimiker/patches
[11]: https://www.qemu.org/
[12]: ../blob/master/Dockerfile
[14]: ../blob/master/install-tools.sh
