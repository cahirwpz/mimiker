Mimiker Compiler Toolchain
=============================

### Prerequisites

Several standard packages are needed to build the toolchain.  

On Debian, executing the following command should suffice:

```
$ sudo apt-get install automake bison ca-certificates curl flex gcc g++ gettext lhasa libtool make patch pkg-config python3 python3-setuptools quilt texinfo xz-utils zip libpython3-dev debhelper
```

### Configuration

You can change the default configuration by editing the file `config.mk`.

### Building

Just enter the main directory and run
```
$ make
```

This process will start by downloading upstream sources, then
will configure, build, and package the toolchain.

**TODO**: this README.md should be moved to the wiki.