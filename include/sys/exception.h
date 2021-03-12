#ifndef _SYS_EXCEPTION_H_
#define _SYS_EXCEPTION_H_

#include <sys/cdefs.h>
#include <sys/sysent.h>
#include <sys/context.h>
#include <sys/ucontext.h>
#include <sys/thread.h>

/* The ctx argument MUST be thread_self()->td_uctx, otherwise the calling
 * thread's kernel stack pointer will be corrupted. */
__noreturn __long_call void _switch_to_uctx(mcontext_t *ctx);

static inline __noreturn void switch_to_userspace(void) {
  _switch_to_uctx(thread_self()->td_uctx);
}

__noreturn __long_call void user_exc_leave(void);

void on_exc_leave(void);
void on_user_exc_leave(mcontext_t *ctx, syscall_result_t *result);

#endif /* !_SYS_EXCEPTION_H_ */
