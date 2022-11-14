Building
---

With [toolchain][1] installed you are ready to compile Mimiker. Before you run
`make` in project's root directory, please have a look at `config.mk` file.
There are several important configurable options there, including:
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

If you need to debug build system, i.e. print commands executed by `make` as
they are run, add `VERBOSE=1` to the command line.

**Important!** It's recommended to issue `make distclean` command after each
modification to `config.mk` file.

**Important!** Before building Mimker for different board you have to run
`make distclean` with `BOARD` argument set to previous board.

[1]: toolchain.md
