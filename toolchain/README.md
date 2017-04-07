Toolchain building procedure
---

 1. Download **release** version of [crosstool-ng](http://crosstool-ng.org/). Using git version is discouraged!
 2. Unpack `crosstool-ng` to a directory and run following commands: 
```
$ ./bootstrap
$ ./configure --prefix=${HOME}/local
$ make && make install
```
 3. Remeber to install `python-dev` package so GDB Python scripting can be enabled.
 4. Provided `${HOME}/local/bin` is in your `PATH` go to `${MIMIKER_TOP}/toolchain/${ARCH}` and run:
```
$ ct-ng build
```

If the build succeeds the toolchain should be installed in `${HOME}/local/${TARGET}`.