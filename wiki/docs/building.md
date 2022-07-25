Building
---

With toolchain installed you are ready to compile Mimiker. Before you run
`make` in project's root directory, please have a look at `config.mk` file.

Of several configurable options, the following are most important to you:
* `VERBOSE=1`: shows commands executed by `make` as they're run, useful when
  debugging build system,
* `KASAN=1`: enables Kernel Address Sanitizer (dynamic memory error detector),
  which finds buffer overruns and other nasty memory related bugs,
* `KCSAN=1`: enables Kernel Concurrency Sanitizer (data races detector), which
  identifies variables that are accessed from multiple threads without proper
  synchronization,
* `LOCKDEP=1`: enables Kernel Lock Dependency checker, which identifies
  violations of locking order that may lead to deadlocks in the kernel,
* `KGPROF=1`: enables kernel profiling, which tracks time spend in each of
  kernel's functions,
* `LLVM` if set to 0 GNU toolchain (gcc & binutils) will be used to compile the
  project instead of LLVM toolchain (clang & lld).

Each option defined with `?=` operator can be overridden from command line. For
example, use `make KASAN=1` command to create a KASAN build.

The result will be a `mimiker.elf` file in `sys` directory containing the kernel
image.
