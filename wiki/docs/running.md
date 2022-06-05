Running
---

We provide a Python script that simplifies running Mimiker OS. The kernel image
is run with QEMU simulator. Several serial consoles are available for
interaction. Optionally you can attach to simulator with `gdb` debugger.
All of that is achieved by running all interactive sessions within
[tmux](https://github.com/tmux/tmux/wiki) terminal multiplexer with default key
bindings.

In project main directory, run command below that will start the kernel in
test-run mode. To finish simulation simply detach from `tmux` session by
pressing `Ctrl+b` and `d` (as in _detach_) keys. To switch between emulated
serial consoles and debugger press `Ctrl+b` and corresponding terminal number.

```
./launch test=all
```

Some useful flags to the `launch` script:

* `-h` - Prints usage.
* `-d` - Starts simulation under a debugger.
* `-t` - Bind simulator UART to current stdio.

Any other argument is passed to the kernel as a kernel command-line
argument. Some useful kernel arguments:

* `init=PROGRAM` - Specifies the userspace program for PID 1.
  Browse `bin` and `usr.bin` directories for currently available programs.
* `klog-quiet=1` - Turns off printing kernel diagnostic messages.

If you want to run tests please read [this document](sys/tests/README.md).

