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
from NetBSD or [suckless](https://suckless.org/) project. We're focused on
kernel development, since we find it more interesting. However we don't want to
invest too much time into the device drivers, so we keep a list of target
platforms small.

If you'd like to get involved in the project please read our
[Wiki](https://github.com/cahirwpz/mimiker/wiki) to find out more!

## Where we are?

Mimiker is real-time operating system, since the kernel is preemptible and our
mutexes support priority inheritance. We minimize work done in interupt context
by delegating it to interrupt threads instead of running it using soft
interrupts. All kernel memory is wired (i.e. non-swappable), so you don't have
to worry about choosing right locks when accessing kernel memory, unlike
in FreeBSD.

Mimiker has nice set of debugging tools: gdb scripts written in Python, Kernel
Address Sanitizer, Lock dependency validator, Kernel Concurrency Sanitizer.

We have support for profiling using gprof!

## What we're proud of ?

Mimiker supports:
 * job control => ksh
 * POSIX terminal, hence it can run [tetris](https://mimiker.ii.uni.wroc.pl/resources/tetris.gif)
 * pseudoterminals => script
 * flat device tree

## Where are we going ?

There's plenty of work to be done. Please refer to our roadmap!
