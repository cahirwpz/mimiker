# WiFire OS (maybe, at some point)
An experiment with implementation of very simple embedded operating system for
ChipKIT WiFire.

Setup
---

Let's assume you'll install all required software in your home directory under
*local* directory.

Firstly you have to install QEMU for MIPS architecture. Version that can
emulate ChipKIT WiFire is available [here](https://github.com/sergev/qemu/wiki).
Remember to install all depencies before you start build process. Following
options should be used to configure *qemu*:

```
./configure --prefix=${HOME}/local --target-list=mipsel-softmmu
```

After you've built and installed *qemu* you need to fetch MIPS toolchain from
Imagination Technologies. It's based on *gcc*, *binutils* and *gdb* - hopefully
you're familiar with hose tools. Yhe installer is available
[here](http://community.imgtec.com/developers/mips/tools/codescape-mips-sdk/).
In my case downloaded file was named
`CodescapeMIPSSDK-1.3.0.42-linux-x64-installer.run`. During installation
process you should choose to install into `${HOME}/local/imgtec` directory. We
need only a cross-compiler for *Bare Metal Applications* and *MIPS Aptiv
Family* of processors.

After toolchain installation you should comment out the last line of
`${HOME}/.imgtec.sh` file (with reference to QEMU path). Instead of that please
add `${HOME}/local/bin` to user's path.

Documentation
---

Useful sites:
* [OSDev wiki](http://wiki.osdev.org)
* [prpl Foundation](http://wiki.prplfoundation.org)
* [Imagination Technologies](http://imgtec.com/)
* [Microchip](http://www.microchip.com/pagehandler/en-us/family/32bit/architecture-pic32mzfamily.html)
* [ChipKIT Development Platform](http://chipkit.net/)

Toolchain documentation:
* [Extensions to the C Language Family](https://gcc.gnu.org/onlinedocs/gcc-4.9.3/gcc/C-Extensions.html)
* [Debugging with GDB](https://sourceware.org/gdb/onlinedocs/gdb/index.html)
* [Linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)

MIPS documentation:
* [MIPS® Architecture For Programmers Volume III: The MIPS32® and microMIPS32™ Privileged Resource Architecture](http://wiki.prplfoundation.org/w/images/d/d2/MD00090-2B-MIPS32PRA-AFP-05.03.pdf)
* [MIPS32® microAptiv™ UP Processor Core Family Software User’s Manual](http://wiki.prplfoundation.org/w/images/5/5b/MD00942-2B-microAptivUP-SUM-01.00.pdf)
* [PIC32MZ Embedded Connectivity (EC) Family](http://ww1.microchip.com/downloads/en/DeviceDoc/60001191D.pdf)
* [Boot-MIPS: Example Boot Code for MIPS® Cores](http://wiki.prplfoundation.org/w/images/6/64/MD00901-2B-CPS-APP-01.03.pdf)
