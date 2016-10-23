#include <common.h>
#include <exec.h>

int main() {
    exec_args_t exec_args;
    exec_args.prog_name = "prog";
    const char* argv[] = {"argument1", "ARGUMENT2", "a-r-g-u-m-e-n-t-3"};
    exec_args.argv = (char**)argv;
    exec_args.argc = 3;

    do_exec(&exec_args);

    return 0;
}
