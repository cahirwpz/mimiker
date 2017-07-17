#ifndef _SYS_EXCEPTION_H_
#define _SYS_EXCEPTION_H_

#include <common.h>
#include <mips/exc.h>

void exc_before_leave(exc_frame_t *kframe);
noreturn void user_exc_leave(void);

#endif /* !_SYS_EXCEPTION_H_ */
