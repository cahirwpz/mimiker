#ifndef __THREAD_H__
#define __THREAD_H__

#include <common.h>
#include <queue.h>
#include <context.h>

typedef uint8_t td_prio_t;
typedef struct vm_page vm_page_t;

typedef struct thread {
  TAILQ_ENTRY(thread) td_runq;    /* a link on run queue */
  TAILQ_ENTRY(thread) td_sleepq;  /* a link on sleep queue */
  td_prio_t td_priority;
  ctx_t td_context;
  vm_page_t *td_stack;
  enum {
    TDS_INACTIVE = 0x0,
    TDS_WAITING,
    TDS_READY,
    TDS_RUNNING
  } td_state;
} thread_t;

_Noreturn void thread_init(void (*fn)(), int argc, ...);

#endif
