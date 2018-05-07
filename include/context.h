#ifndef _SYS_CONTEXT_H_
#define _SYS_CONTEXT_H_

typedef struct ctx ctx_t;
typedef struct thread thread_t;

/*! \brief Initializes context to enter code with given stack. */
void ctx_init(ctx_t *ctx, void *pc, void *sp);

/* This function stores the current context to @from, and resumes the
 * context stored in @to. It does not return immediatelly, it returns
 * only when the @from context is resumed.
 *
 * When switching it atomically releases @from thread spin lock (if applicable)
 * and acquires @to thread spin lock (if TDF_NEEDLOCK is set for it). */
void ctx_switch(thread_t *from, thread_t *to);

#endif /* _SYS_CONTEXT_H_ */
