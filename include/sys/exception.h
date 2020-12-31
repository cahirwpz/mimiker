#ifndef _SYS_EXCEPTION_H_
#define _SYS_EXCEPTION_H_

#include <sys/cdefs.h>
#include <sys/sysent.h>
#include <sys/context.h>

__noreturn __long_call void user_exc_leave(void);

void on_exc_leave(void);
void on_user_exc_leave(mcontext_t *ctx, syscall_result_t *result);

#endif /* !_SYS_EXCEPTION_H_ */
