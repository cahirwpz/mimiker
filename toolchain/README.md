Toolchain building procedure
---

1. Run following command to build the toolchain ready to be installed in root
   filesystem. All files will be installed in `mipsel-mimiker-elf/usr`
   directory.
```
$ ./toolchain-mipsel build
```
2. Go to `mipsel-mimiker-elf` directory and build a Debian package with
   following command:
```
fakeroot ./debian/rules binary
```
