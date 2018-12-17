#define KL_LOG KL_USER
#include <klog.h>
#include <stdc.h>
#include <exec.h>
#include <proc.h>
#include <thread.h>
#include <ktest.h>
#include <malloc.h>
#include <syslimits.h>
#include <stack.h>

/* Borrowed from mips/malta.c */
char *kenv_get(const char *key);

int kmain(void) {
  const char *init = kenv_get("init");
  const char *test = kenv_get("test");

  /* Main kernel thread becomes PID(0) - a god process! */
  (void)proc_create(thread_self(), NULL);

  if (init) {
    const char *argv[] = {init, NULL};
    const size_t max_stack_size = roundup(
      sizeof(size_t) + sizeof(char *) + roundup(strlen(init) + 1, 4), 8);

    int8_t *stack = kmalloc(M_TEMP, max_stack_size, 0);
    size_t stack_size;

    int result =
      kspace_setup_exec_stack(argv, stack, max_stack_size, &stack_size);
    if (result < 0)
      goto end;

    assert(stack_size == max_stack_size);

    exec_args_t init_args = {
      .prog_name = init, .stack_image = stack, .stack_size = stack_size};
#if 0
    exec_args_t init_args = {.prog_name = init,
                             .argv = (const char *[]){init, NULL},
                             .envp = (const char *[]){NULL}};
#endif

    run_program(&init_args);

  end:
    kfree(M_TEMP, stack);
    return result;

  } else if (test) {
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
