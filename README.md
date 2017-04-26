# Mimiker: MIPS Micro-Kernel

An experiment with implementation of very simple operating system
for [Malta](https://www.linux-mips.org/wiki/MIPS_Malta) board.

Building
---

To build Mimiker you will need to prepare a custom MIPS toolchain we use. We
use crosstool-ng for building the toolchain, you can get
it [from here](http://crosstool-ng.org/).

```
cd toolchain/mips/
ct-ng build
```

By default, this will build and install the `mipsel-mimiker-elf` toolchnain to
`~/local`. Update your `$PATH` so that it provides `mipsel-mimiker-elf-*`,
i.e. unless you've changed the install location you will need to append
`~/local/mipsel-mimiker-elf/bin` to your `PATH`.

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

* `-d` - Starts simulation under a debugger.
* `-D DEBUGGER` - Selects debugger to use.
* `-S SIMULATOR` - Manually selects the simulator to use.
* `-t` - Bind simulator UART to current stdio.

Any other argument is passed to the kernel as a kernel command-line
argument. Some useful kernel aguments:

* `init=PROGRAM` - Specifies the userspace program for PID 1. Browse `./user`
  for currently available programs.
* `test=TEST` - Requests the kernel to run the specified test (from `./tests`
  directory).
* `test=all` - Runs a number of tests one after another, and reports success
  only when all of them passed.
* `seed=UINT` - Sets the RNG seed for shuffling the list of test when using
  `test=all`.
* `repeat=UINT` - Specifies the number of (shuffled) repetitions of each test
  when using `test=all`.

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
