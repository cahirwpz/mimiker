#include <common.h>
#include <exec.h>

int main() {
    exec_args_t exec_args;
    exec_args.prog_name = "prog";
    exec_args.argc = 0;

    do_exec(&exec_args);

    return 0;
}
