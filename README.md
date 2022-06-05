# Mimiker: Unix-like system for education and research purposes

Mimiker's main goal is to deliver complete operating system – kernel and a
set of userspace programs, that constitutes a minimal Unix-like system.

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
