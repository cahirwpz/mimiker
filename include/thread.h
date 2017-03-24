#ifndef _SYS_THREAD_H_
#define _SYS_THREAD_H_

#include <common.h>
#include <queue.h>
#include <context.h>
#include <exception.h>
#include <sleepq.h>

typedef uint8_t td_prio_t;
typedef uint32_t tid_t;
typedef struct vm_page vm_page_t;
typedef struct vm_map vm_map_t;
typedef struct fdtab fdtab_t;

#define TD_NAME_MAX 32

#define TDF_SLICEEND 0x00000001   /* run out of time slice */
#define TDF_NEEDSWITCH 0x00000002 /* must switch on next opportunity */

typedef struct thread {
  TAILQ_ENTRY(thread) td_all;    /* a link on all threads list */
  TAILQ_ENTRY(thread) td_runq;   /* a link on run queue */
  TAILQ_ENTRY(thread) td_sleepq; /* a link on sleep queue */
  char *td_name;
  tid_t td_tid;
  /* thread state */
  enum { TDS_INACTIVE = 0x0, TDS_WAITING, TDS_READY, TDS_RUNNING } td_state;
  uint32_t td_flags;           /* TDF_* flags */
  volatile uint32_t td_csnest; /* critical section nest level */
  /* thread context */
  exc_frame_t td_uctx;    /* user context (always exception) */
  fpu_ctx_t td_uctx_fpu;  /* user FPU context (always exception) */
  exc_frame_t *td_kframe; /* kernel context (last exception frame) */
  ctx_t td_kctx;          /* kernel context (switch) */
  intptr_t td_onfault;    /* program counter for copyin/copyout faults */
  vm_page_t *td_kstack_obj;
  stack_t td_kstack;
  vm_map_t *td_uspace; /* thread's user space map */
  /* file descriptors table */
  fdtab_t *td_fdtable;
  /* waiting channel */
  sleepq_t *td_sleepqueue;
  void *td_wchan;
  const char *td_wmesg;
  /* scheduler part */
  td_prio_t td_prio;
  int td_slice;
} thread_t;

thread_t *thread_self();
void thread_init();
thread_t *thread_create(const char *name, void (*fn)(void *), void *arg);
void thread_delete(thread_t *td);

void thread_switch_to(thread_t *td_ready);

noreturn void thread_exit();
void thread_join(void *p);

/* Debugging utility that prints out the summary of all_threads contents. */
void thread_dump_all();

/* Returns the thread matching the given ID, or null if none found. */
thread_t *thread_get_by_tid(tid_t id);

#endif /* _SYS_THREAD_H_ */
