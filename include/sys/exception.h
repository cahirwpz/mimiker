#ifndef _SYS_EXCEPTION_H_
#define _SYS_EXCEPTION_H_

#include <sys/cdefs.h>
#include <sys/types.h>

typedef struct exc_frame exc_frame_t;

__noreturn void user_exc_leave(void);
__noreturn void kernel_oops(exc_frame_t *frame);

/* Flags for \a exc_frame_init */
#define EF_KERNEL 0
#define EF_USER 1

/*! \brief Prepare frame to jump into a program. */
void exc_frame_init(exc_frame_t *frame, void *pc, void *sp, unsigned flags);

/*! \brief Set a return value within the frame and advance the program counter.
 *
 * Useful for returning values from syscalls. */
void exc_frame_set_retval(exc_frame_t *frame, register_t value,
                          register_t error);

/*! \brief Set args and return address for context that calls a procedure. */
void exc_frame_setup_call(exc_frame_t *frame, register_t retaddr,
                          register_t arg0, register_t arg1);

/*! \brief Copy exception frame. */
void exc_frame_copy(exc_frame_t *to, exc_frame_t *from);

void on_exc_leave(void);
void on_user_exc_leave(void);

#endif /* !_SYS_EXCEPTION_H_ */
