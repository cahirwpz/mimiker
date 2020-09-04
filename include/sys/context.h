#ifndef _SYS_CONTEXT_H_
#define _SYS_CONTEXT_H_

typedef struct exc_frame exc_frame_t;
typedef struct thread thread_t;
typedef struct __ucontext ucontext_t;

/*! \brief Initializes context to enter code with given stack. */
void ctx_init(exc_frame_t *ctx, void *pc, void *sp);

/*! \brief Sets a value that will be returned by ctx_switch. */
void ctx_set_retval(exc_frame_t *ctx, long value);

/* This function stores the current context to @from, and resumes the
 * context stored in @to. It does not return immediatelly, it returns
 * only when the @from context is resumed.
 *
 * When switching it atomically releases @from thread spin lock (if applicable)
 * and acquires @to thread spin lock (if TDF_NEEDLOCK is set for it).
 *
 * \returns a value set by \a ctx_set_retval or 0 otherwise.
 */
long ctx_switch(thread_t *from, thread_t *to);

/* Implementation of setcontext syscall. */
int do_setcontext(thread_t *td, ucontext_t *uc);

#endif /* !_SYS_CONTEXT_H_ */
