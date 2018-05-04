#ifndef _SYS_EXCEPTION_H_
#define _SYS_EXCEPTION_H_

#include <common.h>

typedef struct exc_frame exc_frame_t;

void exc_before_leave(exc_frame_t *kframe);
noreturn void user_exc_leave(void);

/* \brief Prepare frame to jump into a program. */
void exc_frame_init(exc_frame_t *frame, vm_addr_t pc, vm_addr_t sp);

/* \brief Set a return value within the frame and advance the program counter.
 *
 * Useful for returning values from syscalls. */
void exc_frame_set_retval(exc_frame_t *frame, long value);

/* \brief Copy exception frame. */
void exc_frame_copy(exc_frame_t *to, exc_frame_t *from);

#endif /* !_SYS_EXCEPTION_H_ */
