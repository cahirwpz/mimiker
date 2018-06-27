#define KL_LOG KL_USER
#include <klog.h>
#include <stdc.h>
#include <exec.h>
#include <ktest.h>
#include <malloc.h>
#include <syslimits.h>

int kspace_marshal_args(const char **user_argv, int8_t *argv_blob,
                        size_t blob_size, size_t *bytes_written);

/* Borrowed from mips/malta.c */
char *kenv_get(const char *key);

int main(void) {
  const char *init = kenv_get("init");
  const char *test = kenv_get("test");

  if (init) {
    /* exec_args_t init_args = { */
    /*   .prog_name = init, .argc = 1, .argv = (const char *[]){init}}; */

    /* run_program(&init_args); */

    const char *argv[] = {init, NULL};
    /* const size_t blob_size = */
    /*   roundup(sizeof(size_t) + sizeof(char*) + roundup( strlen(init) + 1, 4),
     * 8); */

    /* int8_t arg_blob[blob_size]; */

    const size_t blob_size = ARG_MAX;
    int8_t *arg_blob = kmalloc(M_TEMP, blob_size, 0);

    size_t bytes_written;

    kspace_marshal_args(argv, arg_blob, blob_size, &bytes_written);

    // assert(bytes_written == blob_size);
    exec_args_t init_args = {
      .prog_name = init, .arg_blob = arg_blob, .bytes_written = bytes_written};

    run_program(&init_args);

    kfree(M_TEMP, arg_blob);

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
