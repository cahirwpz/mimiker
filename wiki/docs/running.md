Running
---

We provide a Python script, i.e. [launch][3], that simplifies running Mimiker OS.
The kernel image is run with QEMU or Renode simulator. One or more serial
consoles are available for interaction. Optionally you can attach to simulator
with `gdb` debugger.  All of that is achieved by running all interactive
sessions within [tmux][1] terminal multiplexer with default key bindings.

To start kernel in test-run mode, run the following command in project's root
directory. To finish simulation simply detach from `tmux` session by
pressing `Ctrl+b` and `d` (as in _detach_) keys. To switch between emulated
serial consoles and debugger press `Ctrl+b` and corresponding terminal number.

```
./launch test=all
```

Some useful flags to the `launch` script:

* `-h` - Prints usage.
* `-d` - Starts simulation under a debugger.
* `-b` - To specify emulated board (if different than Raspberry Pi 3).
* `-g` - Opens a window with graphics display, if the platform supports it.
* `-k` - Save kft dump to file (`dump.kft`). Works only when compiled with kernel function trace. Can be combined with `-d` flag.

Any other argument is passed to the kernel as a kernel command-line
argument. Some useful kernel arguments:

* `init=PROGRAM` - Specifies the userspace program for PID 1.
  Browse `bin` and `usr.bin` directories for currently available programs.
  In most cases you want to run `/bin/ksh` shell.
* `klog-mask` - Specifies for which [subsystem][4] debug messages will be logged
  to kernel logging facilities. `KL_DEFAULT_MASK` is used by default.
* `klog-utest-mask` - As above but applies to execution of userspace tests.
  `KL_UTEST_MASK` is used by default.

Please note that `launch` script is highly configurable by means of changing
`CONFIG` dictionary.

If you want to run tests please read [this document][2].

[1]: https://github.com/tmux/tmux/wiki
[2]: sys/tests/README.md
[3]: launch
[4]: include/sys/klog.h
