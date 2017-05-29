# Debugging

Custom debugging tools for GDB.

Running
---

After you start kernel in debugging mode, launch script with -d flag
[(detailed how to)](https://github.com/cahirwpz/mimiker#running) you can use follwing custom commands.

* `kdump` - Print on screen current status of kernel.
* `ktrace` - Add/remove breakpoint on selected function.

kdump aguments prints:

* `free_pages` - list of free pages per segment with virtual and physical address
* `segments` - list all memory segments with their start and end address and number of pages (currently just one)
* `klog` - all logs currently saved in kernel
* `threads` - all existing threads
* `tlb` - translation lookaside buffer with addresses and flags marking if page is dirty (D) and/or global (G)

ktrace aguments create/remove breakpoint on:

* `thread-create` - function thread_create in [thread.c](https://github.com/cahirwpz/mimiker/blob/master/sys/thread.c)
* `ctx-switch` - function ctx_switch in [switch.S](https://github.com/cahirwpz/mimiker/blob/master/mips/switch.S)
