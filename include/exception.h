#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <common.h>
#include <mips/exc.h>

void exc_before_leave(exc_frame_t *kframe);
noreturn void user_exc_leave();

#endif // __EXCEPTION_H__
