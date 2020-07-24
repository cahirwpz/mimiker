#include <sys/thread.h>

extern __noreturn void thread_exit(void);
extern __noreturn void kern_exc_leave(void);

void thread_entry_setup(thread_t *td, entry_fn_t target, void *arg) {
  panic("Not implemented!");
}
