#ifndef __UTEST_H__
#define __UTEST_H__

int test_mmap();
int test_sbrk();
int test_misbehave();

int test_fd_read();
int test_fd_devnull();
int test_fd_multidesc();
int test_fd_readwrite();
int test_fd_copy();
int test_fd_bad_desc();
int test_fd_open_path();
int test_fd_dup();
int test_fd_all();

int test_signal_basic();
int test_signal_send();
int test_signal_abort();
int test_signal_segfault();

int test_fork_wait();
int test_fork_signal();
int test_fork_sigchld_ignored();

int test_lseek_set(void);
int test_lseek_cur(void);
int test_lseek_end(void);

#endif /* __UTEST_H__ */
