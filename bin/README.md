How to debug user programs?
---

According to [mimiker.ld](https://github.com/cahirwpz/mimiker/blob/master/user/libmimiker/extra/mimiker.ld)
user space binaries are loaded at *0x400000* address. You could stop an entry
point to the program listed in ELF header. However using **gdb** without
debugging information is not practical in long-term. So... how to stop debugger
at user program `main` procedure and step through it in casual way?

You have to issuing following command (in this case for `utest` program) within
**gdb** session:

> `add-symbol-file user/utest/utest.uelf 0x400000`

... which will load debugging information, so you can use all gdb goodies in
user-space programs.
