#include "utest.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Not enough arguments provided to utest.\n");
    return 1;
  }
  char *test_name = argv[1];
  printf("Starting user test \"%s\".\n", test_name);

#define CHECKRUN_TEST(name)                                                    \
  if (strcmp(test_name, #name) == 0)                                           \
    return test_##name();

  /* Linker set in userspace would be quite difficult to set up, and it feels
     like an overkill to me. */
  CHECKRUN_TEST(mmap);
  CHECKRUN_TEST(munmap_sigsegv);
  CHECKRUN_TEST(sbrk);
  CHECKRUN_TEST(sbrk_sigsegv);
  CHECKRUN_TEST(misbehave);
  CHECKRUN_TEST(fd_read);
  CHECKRUN_TEST(fd_devnull);
  CHECKRUN_TEST(fd_multidesc);
  CHECKRUN_TEST(fd_readwrite);
  CHECKRUN_TEST(fd_copy);
  CHECKRUN_TEST(fd_bad_desc);
  CHECKRUN_TEST(fd_open_path);
  CHECKRUN_TEST(fd_dup);
  CHECKRUN_TEST(fd_pipe);
  CHECKRUN_TEST(fd_all);
  CHECKRUN_TEST(signal_basic);
  CHECKRUN_TEST(signal_send);
  CHECKRUN_TEST(signal_abort);
  CHECKRUN_TEST(signal_segfault);
  CHECKRUN_TEST(signal_stop);
  CHECKRUN_TEST(signal_mask);
  CHECKRUN_TEST(fork_wait);
  CHECKRUN_TEST(fork_signal);
  CHECKRUN_TEST(fork_sigchld_ignored);
  CHECKRUN_TEST(lseek_basic);
  CHECKRUN_TEST(lseek_errors);
  CHECKRUN_TEST(access_basic);
  CHECKRUN_TEST(stat);
  CHECKRUN_TEST(fstat);
  CHECKRUN_TEST(exc_cop_unusable);
  CHECKRUN_TEST(exc_reserved_instruction);
  CHECKRUN_TEST(exc_integer_overflow);
  CHECKRUN_TEST(exc_unaligned_access);
  CHECKRUN_TEST(exc_sigsys);
  CHECKRUN_TEST(syscall_in_bds);
  CHECKRUN_TEST(setjmp);
  CHECKRUN_TEST(sigaction_with_setjmp);
  CHECKRUN_TEST(sigaction_handler_returns);
  CHECKRUN_TEST(vfs_dir);
  CHECKRUN_TEST(vfs_rw);
  CHECKRUN_TEST(vfs_relative_dir);
  CHECKRUN_TEST(vfs_dot_dot_dir);
  CHECKRUN_TEST(vfs_dot_dir);
  CHECKRUN_TEST(vfs_dot_dot_across_fs);
  CHECKRUN_TEST(vfs_trunc);
  CHECKRUN_TEST(vfs_symlink);
  CHECKRUN_TEST(vfs_link);
  CHECKRUN_TEST(vfs_chmod);

  CHECKRUN_TEST(setpgid);
  CHECKRUN_TEST(kill);
  CHECKRUN_TEST(killpg_same_group);
  CHECKRUN_TEST(killpg_other_group);

  CHECKRUN_TEST(fpu_fcsr);
  CHECKRUN_TEST(fpu_gpr_preservation);
  CHECKRUN_TEST(fpu_cpy_ctx_on_fork);
  CHECKRUN_TEST(fpu_ctx_signals);
  CHECKRUN_TEST(getcwd);

  printf("No user test \"%s\" available.\n", test_name);
  return 1;
}
