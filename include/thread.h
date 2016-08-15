#ifndef __THREAD_H__
#define __THREAD_H__

#include <common.h>
#include <queue.h>
#include <context.h>

typedef uint8_t td_prio_t;
typedef struct vm_page vm_page_t;
typedef struct sleepq sleepq_t;

typedef struct thread {
  TAILQ_ENTRY(thread) td_runq;    /* a link on run queue */
  TAILQ_ENTRY(thread) td_sleepq_entry;  /* a link on sleep queue */
  const char *td_name;
  td_prio_t td_priority;
  ctx_t td_context;
  vm_page_t *td_stack;
  sleepq_t *td_sleepqueue; /* points to this thread's sleepqueue. If it's NULL, this thread is sleeping */
  void *td_wchan;
  const char *td_wmesg;
  enum {
    TDS_INACTIVE = 0x0,
    TDS_WAITING,
    TDS_READY,
    TDS_RUNNING
  } td_state;
} thread_t;

thread_t *thread_self();
noreturn void thread_init(void (*fn)(), int n, ...);
thread_t *thread_create(const char *name, void (*fn)());
void thread_delete(thread_t *td);
void thread_switch_to(thread_t *td_ready);
void thread_exit();

#endif // __THREAD_H__
