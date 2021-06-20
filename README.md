# Mimiker: MIPS Micro-Kernel

An experiment with implementation of very simple operating system
for [Malta](https://www.linux-mips.org/wiki/MIPS_Malta) board.

Toolchain
---

To build Mimiker you will need a custom MIPS toolchain we use. You can download
a binary debian package
[from here](http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.2_amd64.deb).
It installs into `/opt`, so you'll need to add `/opt/mipsel-mimiker-elf/bin` to
your `PATH`.

Otherwise, if you prefer to build the toolchain on your own, download
crosstool-ng which we use for configuring the toolchain. You can get
it [from here](http://crosstool-ng.org/). Then:

```
cd toolchain/mips/
ct-ng build
```

By default, this will build and install the `mipsel-mimiker-elf` toolchain to
`~/local`. Update your `$PATH` so that it provides `mipsel-mimiker-elf-*`,
i.e. unless you've changed the install location you will need to append
`~/local/mipsel-mimiker-elf/bin` to your `PATH`.

Building
---

With toolchain in place, you are ready to compile Mimiker. Run

```
make
```

in project root. Currently two additional command-line options are supported:
* `CLANG=1` - Use the Clang compiler instead of GCC (make sure you have it installed!).
* `KASAN=1` - Compile the kernel with the KernelAddressSanitizer, which is a
dynamic memory error detector. 
* `KCSAN=1` - Compile the kernel with the KernelConcurrencySanitizer, a tool for detecting data races.

For example, use `make KASAN=1` command to create a GCC-KASAN build.

The result will be a `mimiker.elf` file containing the kernel image.

Running
---

We provide a Python script that simplifies running Mimiker OS. The kernel image
is run with QEMU simulator. Several serial consoles are available for
interaction. Optionally you can attach to simulator with `gdb` debugger.
All of that is achieved by running all interactive sessions within
[tmux](https://github.com/tmux/tmux/wiki) terminal multiplexer with default key
bindings.

In project main directory, run command below that will start the kernel in
test-run mode. To finish simulation simply detach from `tmux` session by
pressing `Ctrl+b` and `d` (as in _detach_) keys. To switch between emulated
serial consoles and debugger press `Ctrl+b` and corresponding terminal number.

```
./launch test=all
```

Some useful flags to the `launch` script:

* `-h` - Prints usage.
* `-d` - Starts simulation under a debugger.
* `-D DEBUGGER` - Selects debugger to use.
* `-t` - Bind simulator UART to current stdio.

Any other argument is passed to the kernel as a kernel command-line
argument. Some useful kernel arguments:

* `init=PROGRAM` - Specifies the userspace program for PID 1.
  Browse `bin` and `usr.bin` directories for currently available programs.
* `klog-quiet=1` - Turns off printing kernel diagnostic messages.

If you want to run tests please read [this document](sys/tests/README.md).

Documentation
---

Useful sites:
* [OSDev wiki](http://wiki.osdev.org)

Toolchain documentation:
* [Extensions to the C Language Family](https://gcc.gnu.org/onlinedocs/gcc-4.9.3/gcc/C-Extensions.html)
* [Debugging with GDB](https://sourceware.org/gdb/onlinedocs/gdb/index.html)
* [Linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)

MIPS documentation:
* [MIPS® Architecture For Programmers Volume II-A: The MIPS32® Instruction Set](http://mimiker.ii.uni.wroc.pl/documents/MD00086-2B-MIPS32BIS-AFP-6.06.pdf)
* [MIPS® Architecture For Programmers Volume III: The MIPS32® and microMIPS32™ Privileged Resource Architecture](http://mimiker.ii.uni.wroc.pl/documents/MD00090-2B-MIPS32PRA-AFP-06.02.pdf)
* [MIPS32® 24KE™ Processor Core Family Software User’s Manual](http://mimiker.ii.uni.wroc.pl/documents/MD00468-2B-24KE-SUM-01.11.pdf)
* [MIPS32® 24KEf™ Processor Core Datasheet](http://mimiker.ii.uni.wroc.pl/documents/MD00446-2B-24KEF-DTS-02.00.pdf)
* [Programming the MIPS32® 24KE™ Core Family](http://mimiker.ii.uni.wroc.pl/documents/MD00458-2B-24KEPRG-PRG-04.63.pdf)
* [MIPS® YAMON™ User’s Manual](http://mimiker.ii.uni.wroc.pl/documents/MD00008-2B-YAMON-USM-02.19.pdf)
* [MIPS® YAMON™ Reference Manual](http://mimiker.ii.uni.wroc.pl/documents/MD00009-2B-YAMON-RFM-02.20.pdf)
* [MIPS ABI Project](https://dmz-portal.mips.com/wiki/MIPS_ABI_Project)

Hardware documentation:
* [MIPS® Malta™-R Development Platform User’s Manual](http://mimiker.ii.uni.wroc.pl/documents/MD00627-2B-MALTA_R-USM-01.01.pdf)
* [Galileo GT–64120 System Controller](http://doc.chipfind.ru/pdf/marvell/gt64120.pdf)
* [Intel® 82371AB PCI-TO-ISA/IDE XCELERATOR (PIIX4)](http://www.intel.com/assets/pdf/datasheet/290562.pdf)
* [Am79C973: Single-Chip 10/100 Mbps PCI Ethernet Controller with Integrated PHY](http://pdf.datasheetcatalog.com/datasheet/AdvancedMicroDevices/mXwquw.pdf)
* [FDC37M81x: PC98/99 Compliant Enhanced Super I/O Controller with Keyboard/Mouse Wake-Up](http://www.alldatasheet.com/datasheet-pdf/pdf/119979/SMSC/FDC37M817.html)
