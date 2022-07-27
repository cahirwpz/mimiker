# Mimiker: Unix-like system for education and research purposes

Mimiker's main goal is to deliver minimal Unix-like operating system, i.e.
the kernel and a set of userspace programs.

Kernel design is heavily inspired by FreeBSD & NetBSD systems with some ideas
taken from Linux, Plan9 and other OSes. We spend a lot of time reading source
code of open-source operating systems. We carefully choose their best design
decisions, ideas, algorithms, APIs, practices and so on, distill them to bare
minimum and reimplement them or adapt to Mimiker code base. We hope not to
repeat their mistakes and move away from legacy and non-perfect solutions.

Mimiker project gathers like minded people who value minimalism, simplicity and
readability of code. We strive for the lowest possible complexity of solutions.
We love to throw away code that isn't terribly useful or handles rare edge
cases. We know value of debuggability and we don't hesitate to spend time
writing tools that help to improve it.

Though userspace programs are part of Mimiker project, they've got simply ported
from NetBSD or [suckless][1] project. We focus on kernel development, since we
find it more interesting. We don't want to invest too much time into the device
drivers, so we keep a list of target platforms small.

If you'd like to get involved in the project please read our [Wiki][2] to find
out more!

## Where we are

Mimiker is a real-time operating system. The kernel is preemptible and our
mutexes support priority inheritance. We minimize work done in interrupt context
by delegating it to interrupt threads instead of running it using soft
interrupts.

Mimiker runs on [MIPS][15] (32-bit), [AArch64][9] and [RISC-V][10] (both 32-bit
and 64-bit) architectures under [QEmu][11] and [Renode][12] control.

Mimiker has nice set of debugging tools: `gdb` scripts written in Python, Kernel
Address Sanitizer, Lock dependency validator, Kernel Concurrency Sanitizer. We
even have support for profiling the kernel using `gprof`! We use [Clang][19] to
compile our code base, hence we can employ sophisticated dynamic and static
analysis algorithms to aid code reliablity.

A common set of synchronization primitives is provided, i.e. spin-locks, mutexes
and conditional variables - all with simple semantics. We don't have multiple
primitives that do similar things, but a little bit differently, which is common
for FreeBSD or Linux kernels.

Mimiker's kernel memory is wired (i.e. non-swappable), so you don't have to
worry about choosing right locks when accessing kernel memory, unlike in
FreeBSD.  We have buddy memory allocator for physical memory, virtual address
space allocator and slab allocator based on [Magazines and Vmem][3] paper. Our
memory allocators are simple yet efficient.

Mimiker's driver infrastructure abstracts away concept of hardware register
and interrupts in similar manner to FreeBSD's [NewBus][14]. Special care is
taken to make drivers portable. We have enumerator routines that autodetect
devices attached to PCI and USB buses. We use [flat device tree][13] to drive
kernel configuration during startup phase.

Virtual file system and user virtual address space management are loosely based
on FreeBSD ideas. They need substatial amount of work to become as mature as in
FreeBSD or Linux kernels.

## What we are proud of

We have over eighty [syscalls][4] that allow us to run various open-source
tools, including NetBSD's [Korn Shell][5], [Atto Emacs][6] editor, [Lua][7]
interpreter, and many more. We even have a game:

![tetris][8]

Mimiker supports:
 * UNIX file I/O -- well known APIs for file-like objects access,
 * interprocess communication -- POSIX signal and pipes,
 * job control -- thus we can run unmodified [Korn Shell][18],
 * UNIX credentials -- users, groups, file permissions,
 * libterminfo, hence Mimiker can run some fullscreen terminal applications,
 * [pseudoterminals][16] -- so we can run [script][17] or terminal emulators.

## What is missing

We would like to support:
 * multi-core systems,
 * VirtIO and virt platforms in QEmu,
 * a filesystem for non-volatile storage devices,
 * TCP/IP protocols.

There's plenty of work to be done. Please refer to our roadmap!

[1]: https://suckless.org
[2]: https://github.com/cahirwpz/mimiker/wiki
[3]: https://www.usenix.org/legacy/publications/library/proceedings/usenix01/full_papers/bonwick/bonwick.pdf
[4]: https://github.com/cahirwpz/mimiker/blob/master/sys/kern/syscalls.master
[5]: https://man.netbsd.org/ksh.1
[6]: https://github.com/hughbarney/atto
[7]: https://www.lua.org/docs.html
[8]: https://mimiker.ii.uni.wroc.pl/resources/tetris.gif
[9]: https://www.qemu.org/docs/master/system/target-arm.html
[10]: https://www.qemu.org/docs/master/system/target-riscv.html
[11]: https://www.qemu.org
[12]: https://renode.io
[13]: https://wiki.freebsd.org/FlattenedDeviceTree
[14]: https://nostarch.com/download/samples/freebsd-device-drivers_ch7.pdf
[15]: https://www.qemu.org/docs/master/system/target-mips.html
[16]: https://en.wikipedia.org/wiki/Pseudoterminal
[17]: https://man.netbsd.org/script.1
[18]: https://man.netbsd.org/ksh.1
[19]: https://clang.llvm.org/
