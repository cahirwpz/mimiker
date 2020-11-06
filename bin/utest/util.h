
/* Wait for a single child process with process id `pid` to exit,
 * and check that its exit code matches the expected value. */
void wait_for_child_exit(int pid, int exit_code);

/* Do the necessary setup needed to wait for the signal.
 * Must be called before receiving the signal,
 * ideally at the very beginning of the test procedure. */
void signal_setup(int signo);

/* Wait for the delivery of a signal. */
void wait_for_signal(int signo);
