#include <common.h>
#include <exec.h>

int main() {
    // This is a simple demonstration of the exec functionality. It
    // requests to substitute current thread's image with program
    // called "prog", which is implemented in ./user/prog.c, and after
    // compilation embedded in the kernel image.

    // As the loaded program has no means to communicate with the
    // system, because system calls are not yet implemented, it runs
    // quietly. To test its behavior with a debugger, after starting
    // gdb issue the command:
    //    add-symbol-file user/prog.uelf 0x00400000
    // which will load the symbols for the user program. You may then
    // e.g. break at its main(), and see that argc and argv are set
    // correctly. If you let it run further, you may `print textarea`
    // to see that it accesses a string in .data and copies it to
    // .bss.

    exec_args_t exec_args;
    exec_args.prog_name = "prog";
    const char* argv[] = {"argument1", "ARGUMENT2", "a-r-g-u-m-e-n-t-3"};
    exec_args.argv = (char**)argv;
    exec_args.argc = 3;

    do_exec(&exec_args);

    return 0;
}
