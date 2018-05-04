#ifndef _SYS_CONTEXT_H_
#define _SYS_CONTEXT_H_

#include <common.h>
#include <mips/ctx.h>

typedef struct stack {
  void *stk_base;  /* stack base */
  size_t stk_size; /* stack length */
} stack_t;

typedef struct thread thread_t;

/*
 * Initializes thread context so it can be resumed in such a way,
 * as if @target function was called with @arg argument.
 *
 * Such thread can be resumed either by switch or return from exception.
 *
 * TODO: A thread should call `thread_exit(NULL)` on return.
 */
void ctx_init(thread_t *td, void (*target)(void *), void *arg);

/* This function stores the current context to @from, and resumes the
 * context stored in @to. It does not return immediatelly, it returns
 * only when the @from context is resumed.
 *
 * When switching it atomically releases @from thread spin lock (if applicable)
 * and acquires @to thread spin lock (if TDF_NEEDLOCK is set for it). */
void ctx_switch(thread_t *from, thread_t *to);

#endif /* _SYS_CONTEXT_H_ */
