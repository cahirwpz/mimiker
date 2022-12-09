#include <setjmp.h>
#include <signal.h>

/* Wait for a single child process with process id `pid` to exit,
 * and check that its exit code matches the expected value. */
void wait_for_child_exit(int pid, int exit_code);

/* Do the necessary setup needed to wait for the signal.
 * Must be called before receiving the signal,
 * ideally at the very beginning of the test procedure. */
void signal_setup(int signo);

/* Wait for the delivery of a signal. */
void wait_for_signal(int signo);

/* Create a new pseudoterminal and return file descriptors to the master and
 * slave side. */
void open_pty(int *master_fd, int *slave_fd);

extern jmp_buf signal_return;

void setup_handler(int signo, siginfo_t *siginfo_ptr);
void check_signal(int signo, void *addr, int code);
void restore_handler(void);

#define EXPECT_SIGNAL(signo, siginfo_ptr)                                      \
  for (setup_handler((signo), (siginfo_ptr));                                  \
       (sigsetjmp(signal_return, 1) == 0); assert(0))

#define CLEANUP_SIGNAL() restore_handler()

#define CHECK_SIGSEGV(si, sig_addr, sig_code)                                  \
  ({                                                                           \
    assert((si)->si_addr == (sig_addr));                                       \
    assert((si)->si_code == (sig_code));                                       \
  })
