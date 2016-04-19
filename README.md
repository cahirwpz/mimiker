# Mimiker: MIPS Micro-Kernel

An experiment with implementation of very simple operating system for [Malta](https://www.linux-mips.org/wiki/MIPS_Malta) board.

Setup
---

Let's assume you'll install all required software in your home directory under
*local* directory.

1. Register at [ovpworld.org](http://www.ovpworld.org/forum/profile.php?mode=register)

2. Download OVPsim simulator package for Linux, release
   [20160323](http://www.ovpworld.org/dlp/?action=dl&dl_id=23612&site=dlp.OVPworld.org)

3. Unpack the downloaded package `OVPsim.20160323.0.Linux32.exe`.
   Install the unpacked directory as: `${HOME}/local/Imperas.20160323`.

4. Add to `.bashrc` following lines:

   ```
   source ${HOME}/local/Imperas.20160323.0/bin/setup.sh
   setupImperas ${HOME}/local/Imperas.20160323.0 -m32 
   ```

5. Get your host ID, needed for a license:

   ```
   $ $IMPERAS_HOME/bin/Linux32/lmutil lmhostid
   lmutil - Copyright (c) 1989-2011 Flexera Software, Inc. All Rights Reserved.
   The FLEXnet host ID of this machine is "00123f7c25ed"
   ```

6. Fill a [form](http://www.ovpworld.org/likey/) and send request for OVPsim
   license. Use host ID from previous step. Ask for a free license for
   personal non-commercial usage.

7. When license received by email, put it to the file
   `$IMPERAS_HOME/OVPsim.lic`.

After you've built and installed *OVPsim* you need to fetch MIPS toolchain from
Imagination Technologies. It's based on *gcc*, *binutils* and *gdb* - hopefully
you're familiar with these tools. The installer is available
[here](http://community.imgtec.com/developers/mips/tools/codescape-mips-sdk/).
In my case downloaded file was named
`CodescapeMIPSSDK-1.3.0.42-linux-x64-installer.run`. During installation
process you should choose to install into `${HOME}/local/imgtec` directory. We
need only a cross-compiler for *Bare Metal Applications* and *MIPS Classic
Legacy CPU IP Cores*.

After toolchain installation you should comment out the last line of
`${HOME}/.imgtec.sh` file (with reference to QEMU path). Instead of that please
add `${HOME}/local/bin` to user's path.

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
* [MIPS® Architecture For Programmers Volume III: The MIPS32® and microMIPS32™ Privileged Resource Architecture](http://wiki.prplfoundation.org/w/images/d/d2/MD00090-2B-MIPS32PRA-AFP-05.03.pdf)
* [MIPS32® 24KE™ Processor Core Family Software User’s Manual](http://wiki.prplfoundation.org/w/images/8/83/MD00468-2B-24KE-SUM-01.11.pdf)
* [MIPS32® 24KEf™ Processor Core Datasheet](http://wiki.prplfoundation.org/w/images/9/9c/MD00446-2B-24KEF-DTS-02.00.pdf)
* [Programming the MIPS32® 24KE™ Core Family](http://wiki.prplfoundation.org/w/images/2/20/MD00458-2B-24KEPRG-PRG-04.63.pdf)
* [MIPS® YAMON™ User’s Manual](http://wiki.prplfoundation.org/w/images/b/b9/MD00008-2B-YAMON-USM-02.19.pdf)
* [MIPS ABI Project](https://dmz-portal.mips.com/wiki/MIPS_ABI_Project)

Hardware documentation:
* [MIPS® Malta™-R Development Platform User’s Manual](http://wiki.prplfoundation.org/w/images/4/47/MD00627-2B-MALTA_R-USM-01.01.pdf)
* [Galileo GT–64120 System Controller](http://doc.chipfind.ru/pdf/marvell/gt64120.pdf)
