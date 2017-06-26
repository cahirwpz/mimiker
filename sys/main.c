#define KL_LOG KL_USER
#include <klog.h>
#include <stdc.h>
#include <exec.h>
#include <filedesc.h>
#include <proc.h>
#include <thread.h>
#include <ktest.h>
#include <malloc.h>
#include <vfs.h>

/* Borrowed from mips/malta.c */
char *kenv_get(const char *key);

static void run_init(const char *program) {
  klog("Starting program \"%s\"", program);

  thread_t *td = thread_self();

  /* This kernel thread will become a main thread of the first user process. */
  proc_t *p = proc_create();
  proc_populate(p, td);

  /* Let's assign an empty virtual address space, to be filled by `do_exec` */
  p->p_uspace = vm_map_new();

  /* Prepare file descriptor table... */
  fdtab_t *fdt = fdtab_alloc();
  fdtab_ref(fdt);
  td->td_proc->p_fdtable = fdt;

  /* ... and initialize file descriptors required by the standard library. */
  int ignore;
  do_open(td, "/dev/cons", O_RDONLY, 0, &ignore);
  do_open(td, "/dev/cons", O_WRONLY, 0, &ignore);
  do_open(td, "/dev/cons", O_WRONLY, 0, &ignore);

  exec_args_t exec_args;
  exec_args.prog_name = program;
  exec_args.argv = (const char *[]){program};
  exec_args.argc = 1;

  if (do_exec(&exec_args))
    panic("Failed to start init program.");

  user_exc_leave();
}

int main(void) {
  const char *init = kenv_get("init");
  const char *test = kenv_get("test");

  if (init)
    run_init(init);
  else if (test) {
    ktest_main(test);
  } else {
    /* This is a message to the user, so I intentionally use kprintf instead of
     * log. */
    kprintf("============\n");
    kprintf("No init specified!\n");
    kprintf("Use init=PROGRAM to start a user-space init program or test=TEST "
            "to run a kernel test.\n");
    kprintf("============\n");
    return 1;
  }

  return 0;
}
