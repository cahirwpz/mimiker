# WiFire OS (maybe, at some point)
An experiment with implementation of very simple embedded operating system for ChipKIT WiFire.

Setup
===

Let's assume you'll install all required software in your home directory under *local* directory.

Firstly you have to install QEMU for MIPS architecture. Version that can emulate ChipKIT WiFire is available [here]( https://github.com/sergev/qemu/wiki). Remember to install all depencies before you start build process. Following options should be used to configure *qemu*:

```
./configure --prefix=${HOME}/local --target-list=mipsel-softmmu
```

After you've built and installed *qemu* you need to fetch MIPS toolchain from Imagination Technologies. It's based on *gcc*, *binutils* and *gdb* - hopefully you're familiar with hose tools. Yhe installer is available [here](http://community.imgtec.com/developers/mips/tools/codescape-mips-sdk/). In my case downloaded file was named  `CodescapeMIPSSDK-1.3.0.42-linux-x64-installer.run`. During installation process you should choose to install into `${HOME}/local/imgtec` directory. We need only a cross-compiler for *Bare Metal Applications* and *MIPS Aptiv Family* of processors.

After toolchain installation you should comment out the last line of `${HOME}/.imgtec.sh` file (with reference to QEMU path). Instead of that please add `${HOME}/local/bin` to user's path.
