# Mimiker: MIPS Micro-Kernel

An experiment with implementation of very simple operating system
for [Malta](https://www.linux-mips.org/wiki/MIPS_Malta) board.

Toolchain
---

To build Mimiker you will need a custom MIPS toolchain we use. You can download
a binary debian package
[from here](http://mimiker.ii.uni.wroc.pl/download/mipsel-mimiker-elf_1.1_amd64.deb).
It installs into `/opt`, so you'll need to add `/opt/mipsel-mimiker-elf/bin` to
your `PATH`.

Otherwise, if you prefer to build the toolchain on your own, download
crosstool-ng which we use for configuring the toolchain. You can get
it [from here](http://crosstool-ng.org/). Then:

```
cd toolchain/mips/
ct-ng build
```

By default, this will build and install the `mipsel-mimiker-elf` toolchnain to
`~/local`. Update your `$PATH` so that it provides `mipsel-mimiker-elf-*`,
i.e. unless you've changed the install location you will need to append
`~/local/mipsel-mimiker-elf/bin` to your `PATH`.

Building
---

With toolchain in place, you are ready to compile Mimiker. Run

```
make
```

in project root. This will result with a `mimiker.elf` file containing the
kernel image.

Running
---

As you presumably do not own a MIPS Malta board, you will need a simulator to
test the kernel. We currently support *OVPsim* (incl. Imperas proprietary
variant) and QEMU. If you're using OVPsim, make sure your `$IMPERAS_HOME` is set
correctly.

We provide a python script which simplifies loading the kernel image to
simulator. In project root dir, run:

```
./launch
```

This will start the kernel using OVPsim if available, or QEMU otherwise.

Some useful flags to the `launch` script:

* `-h` - Prints usage.
* `-d` - Starts simulation under a debugger.
* `-D DEBUGGER` - Selects debugger to use.
* `-S SIMULATOR` - Manually selects the simulator to use.
* `-t` - Bind simulator UART to current stdio.

Any other argument is passed to the kernel as a kernel command-line
argument. Some useful kernel aguments:

* `init=PROGRAM` - Specifies the userspace program for PID 1. Browse `./user`
  for currently available programs.
* test-related arguments described in the [following section](#test-infrastructure).

Test infrastructure
---

##### User tests
Located in `/user/utest`.
User-space test function signature looks like this: `int test_{name}(void)` 
and should be defined in `user/utest/utest.h`.
In order to make the test runnable one has to add one of these lines to `test/utest.c` file:
* `UTEST_ADD_SIMPLE({name})` - test fails on assertion or non-zero return value.
* `UTEST_ADD_SIGNAL({name}, {SIGNUMBER})` - test passes when terminated with `{SIGNUMBER}`.
* `UTEST_ADD({name}, {exit status}, flags)` - test passes when exited with status `{exit status}`.

One also needs to add a line `CHECKRUN_TEST({name})` in `/user/utest/main.c`.

##### Kernel tests 
Located in `/tests`. 
Test function signature looks like this:
`{name}(void)` or sometimes `{name}(unsigned int)` but needs to be casted to 
`(int (*)(void))`.

Macros to register tests:
* `KTEST_ADD(name, func, flags)`
* `KTEST_ADD_RANDINT(name, func, flags, max)` - need to cast function pointer to
`(int (*)(void))`

Where `name` is test name, `func` is pointer to test function, 
flags as mentioned bellow, and `max` is maximum random argument fed to the test.

##### Flags
* `KTEST_FLAG_NORETURN` - signifies that a test does not return.
* `KTEST_FLAG_DIRTY` - signifies that a test irreversibly breaks internal kernel state, and any
   further test done without restarting the kernel will be inconclusive.
* `KTEST_FLAG_USERMODE` - indicates that a test enters usermode.
* `KTEST_FLAG_BROKEN` - excludes the test from being run in auto mode. This flag is only useful for
   temporarily marking some tests while debugging the testing framework.
* `KTEST_FLAG_RANDINT` - marks that the test wishes to receive a random integer as an argument.

###### `./launch` test-related arguments:
* `test=TEST` - Requests the kernel to run the specified test.
`test=user_{name}` to run single user test, `test={name}` to run single kernel test.
* `test=all` - Runs a number of tests one after another, and reports success
  only when all of them passed.
* `seed=UINT` - Sets the RNG seed for shuffling the list of test when using
  `test=all`.
* `repeat=UINT` - Specifies the number of (shuffled) repetitions of each test
  when using `test=all`.

###### `./run_tests.py` usefull arguments:
* `-h` - prints script usage.
* `--infinite` - keep testing until some error is found. 
* `--non-interactive` - do not run interactive gdb session if tests fail.
* `--thorough` - generate much more test seeds. Testing will take much more time.

Documentation
---

Useful sites:
* [OSDev wiki](http://wiki.osdev.org)
* [prpl Foundation](http://wiki.prplfoundation.org)

Toolchain documentation:
* [Extensions to the C Language Family](https://gcc.gnu.org/onlinedocs/gcc-4.9.3/gcc/C-Extensions.html)
* [Debugging with GDB](https://sourceware.org/gdb/onlinedocs/gdb/index.html)
* [Linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)

MIPS documentation:
* [MIPS® Architecture For Programmers Volume II-A: The MIPS32® Instruction Set](http://wiki.prplfoundation.org/w/images/1/1b/MD00086-2B-MIPS32BIS-AFP-06.02.pdf)
* [MIPS® Architecture For Programmers Volume III: The MIPS32® and microMIPS32™ Privileged Resource Architecture](http://wiki.prplfoundation.org/w/images/d/d2/MD00090-2B-MIPS32PRA-AFP-05.03.pdf)
* [MIPS32® 24KE™ Processor Core Family Software User’s Manual](http://wiki.prplfoundation.org/w/images/8/83/MD00468-2B-24KE-SUM-01.11.pdf)
* [MIPS32® 24KEf™ Processor Core Datasheet](http://wiki.prplfoundation.org/w/images/9/9c/MD00446-2B-24KEF-DTS-02.00.pdf)
* [Programming the MIPS32® 24KE™ Core Family](http://wiki.prplfoundation.org/w/images/2/20/MD00458-2B-24KEPRG-PRG-04.63.pdf)
* [MIPS® YAMON™ User’s Manual](http://wiki.prplfoundation.org/w/images/b/b9/MD00008-2B-YAMON-USM-02.19.pdf)
* [MIPS® YAMON™ Reference Manual](http://wiki.prplfoundation.org/w/images/8/80/MD00009-2B-YAMON-RFM-02.20.pdf)
* [MIPS ABI Project](https://dmz-portal.mips.com/wiki/MIPS_ABI_Project)

Hardware documentation:
* [MIPS® Malta™-R Development Platform User’s Manual](http://wiki.prplfoundation.org/w/images/4/47/MD00627-2B-MALTA_R-USM-01.01.pdf)
* [Galileo GT–64120 System Controller](http://doc.chipfind.ru/pdf/marvell/gt64120.pdf)
* [Intel® 82371AB PCI-TO-ISA/IDE XCELERATOR (PIIX4)](http://www.intel.com/assets/pdf/datasheet/290562.pdf)
* [Am79C973: Single-Chip 10/100 Mbps PCI Ethernet Controller with Integrated PHY](http://pdf.datasheetcatalog.com/datasheet/AdvancedMicroDevices/mXwquw.pdf)
* [FDC37M81x: PC98/99 Compliant Enhanced Super I/O Controller with Keyboard/Mouse Wake-Up](http://www.alldatasheet.com/datasheet-pdf/pdf/119979/SMSC/FDC37M817.html)
