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
We love to throw away code that isn't terribly useful or handles infrequent edge
cases. We know value of debuggability and we don't hesitate to spend time
writing tools that help to improve it.

Though userspace programs are part of Mimiker project, they've got simply ported
from NetBSD or [suckless][1] project. We focus on kernel development, since we
find it more interesting. However we don't want to invest too much time into the
device drivers, so we keep a list of target platforms small.

If you'd like to get involved in the project please read our [Wiki][2] to find
out more!

## Where we are?

Mimiker is real-time operating system. The kernel is preemptible and our mutexes
support priority inheritance. We minimize work done in interupt context by
delegating it to interrupt threads instead of running it using soft interrupts.

All kernel memory is wired (i.e. non-swappable), so you don't have to worry
about choosing right locks when accessing kernel memory, unlike in FreeBSD.
We have buddy memory allocator for physical memory, virtual address space
allocator and slab allocator based on [Magazines and Vmem][3] paper.

Mimiker has nice set of debugging tools: gdb scripts written in Python, Kernel
Address Sanitizer, Lock dependency validator, Kernel Concurrency Sanitizer.

We have support for profiling using gprof!

## What we're proud of ?

We have over eighty [syscalls][4] that allow us to run various open-source
tools, including NetBSD's [Korn Shell][5], [Atto Emacs][6] editor, [Lua][7]
interpreter, and many more. We even have a game:

![tetris][8]

Mimiker supports:
 * job control => ksh
 * POSIX terminal, hence it can run 
 * pseudoterminals => script
 * flat device tree

## Where are we going ?

There's plenty of work to be done. Please refer to our roadmap!

[1]: https://suckless.org
[2]: https://github.com/cahirwpz/mimiker/wiki
[3]: https://www.usenix.org/legacy/publications/library/proceedings/usenix01/full_papers/bonwick/bonwick.pdf
[4]: https://github.com/cahirwpz/mimiker/blob/master/sys/kern/syscalls.master
[5]: https://man.netbsd.org/ksh.1
[6]: https://github.com/hughbarney/atto
[7]: https://www.lua.org/docs.html
[8]: https://mimiker.ii.uni.wroc.pl/resources/tetris.gif
