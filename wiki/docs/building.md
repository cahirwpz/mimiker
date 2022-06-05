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

