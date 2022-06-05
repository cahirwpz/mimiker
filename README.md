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
* [Extensions to the C Language Family](https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html)
* [Debugging with GDB](https://sourceware.org/gdb/onlinedocs/gdb/index.html)
* [Linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)
