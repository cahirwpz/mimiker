#include <ktest.h>
#include <file.h>
#include <filedesc.h>
#include <vfs_syscalls.h>
#include <thread.h>
#include <proc.h>
#include <sched.h>
#include <clock.h>

static int test_dup() {
  thread_t test_thread;
  proc_t test_proc;
  fdtab_t *test_fdt = fdtab_alloc();
  test_proc.p_fdtable = test_fdt;
  test_thread.td_proc = &test_proc;
  file_t *first_file = file_alloc();

  int first_fd = 0;
  int second_fd = 0;

  fdtab_install_file(test_fdt, first_file, &first_fd);
  kprintf("Successfully installed file, fd = %d\n", first_fd);

  do_dup(&test_thread, first_fd, &second_fd);

  file_t *second_file;
  fdtab_get_file(test_fdt, second_fd, 0, &second_file);

  assert(first_file == second_file);
  kprintf("Successfully dup'ed file, new fd = %d\n", second_fd);

  do_dup2(&test_thread, first_fd, second_fd);
  fdtab_get_file(test_fdt, second_fd, 0, &second_file);
  assert(first_file == second_file);

  return KTEST_SUCCESS;
}

KTEST_ADD(dup, test_dup, 0);
