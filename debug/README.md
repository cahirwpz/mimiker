# Debugging

Custom debugging tools for GDB.

Running
---

After you start the kernel in debugging mode, i.e. using
[launch](https://github.com/cahirwpz/mimiker#running) script with `-d` flag,
you can use following custom commands:

* `kdump` - Print on screen current state of kernel structures,
* `ktrace` - Add/remove breakpoint on selected function.
* `kmem` - Dump info on kernel memory pools.

`kdump` parameter select which state to dump:

* `free_pages` - list of free pages per segment with virtual and physical
   addresses,
* `segments` - list all memory segments, incl. start, end addresses and
   number of pages (currently just one),
* `klog` - all log messages currently saved in the kernel,
* `threads` - all existing threads,
* `tlb` - Translation Lookaside Buffer with addresses and flags marking if page
  is dirty (D) and/or global (G).

`ktrace` parameter selects on which predefined function create/remove
breakpoints on:

* `thread-create` - function [thread_create](https://github.com/cahirwpz/mimiker/blob/master/sys/thread.c)
* `ctx-switch` - function [ctx_switch](https://github.com/cahirwpz/mimiker/blob/master/mips/switch.S)

`kmem` parameter is a name of a pool, either declared globally (most
likely of the form M_*, e.g. M_TEMP) or locally in current scope. To
see a list of available pools use autocompletion feature, i.e. write
`kmem <TAB><TAB>`.