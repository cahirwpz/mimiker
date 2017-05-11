#ifndef _SYS_THREAD_H_
#define _SYS_THREAD_H_

#include <common.h>
#include <queue.h>
#include <context.h>
#include <exception.h>
#include <sleepq.h>
#include <mutex.h>
#include <condvar.h>
#include <signal.h>

typedef uint8_t td_prio_t;
typedef struct vm_page vm_page_t;
typedef struct vm_map vm_map_t;
typedef struct fdtab fdtab_t;
typedef struct proc proc_t;

#define TD_NAME_MAX 32

#define TDF_SLICEEND 0x00000001   /* run out of time slice */
#define TDF_NEEDSWITCH 0x00000002 /* must switch on next opportunity */
#define TDF_NEEDSIGCHK                                                         \
  0x00000004 /* signals were posted since last exc return */

typedef struct thread {
  /* Locks */
  mtx_t td_lock;
  condvar_t td_waitcv; /* CV for thread exit, used by join */
  /* List links */
  TAILQ_ENTRY(thread) td_all;     /* a link on all threads list */
  TAILQ_ENTRY(thread) td_runq;    /* a link on run queue */
  TAILQ_ENTRY(thread) td_sleepq;  /* a link on sleep queue */
  TAILQ_ENTRY(thread) td_zombieq; /* a link on zombie queue */
  TAILQ_ENTRY(thread) td_procq;   /* a link on process threads queue */
  /* Properties */
  proc_t *td_proc; /* Parent process, NULL for kernel threads. */
  char *td_name;
  tid_t td_tid;
  /* thread state */
  enum { TDS_INACTIVE = 0x0, TDS_WAITING, TDS_READY, TDS_RUNNING } td_state;
  uint32_t td_flags;           /* TDF_* flags */
  volatile uint32_t td_csnest; /* critical section nest level */
  int td_exitcode;
  /* thread context */
  exc_frame_t td_uctx;    /* user context (always exception) */
  fpu_ctx_t td_uctx_fpu;  /* user FPU context (always exception) */
  exc_frame_t *td_kframe; /* kernel context (last exception frame) */
  ctx_t td_kctx;          /* kernel context (switch) */
  intptr_t td_onfault;    /* program counter for copyin/copyout faults */
  vm_page_t *td_kstack_obj;
  stack_t td_kstack;
  /* waiting channel */
  sleepq_t *td_sleepqueue;
  void *td_wchan;
  const char *td_wmesg;
  /* scheduler part */
  td_prio_t td_prio;
  int td_slice;
  sigset_t td_sigpend; /* Pending signals for this thread. */
  /* TODO: Signal mask, sigsuspend. */
} thread_t;

void thread_init();

thread_t *thread_self();
thread_t *thread_create(const char *name, void (*fn)(void *), void *arg);
void thread_delete(thread_t *td);

/* Exit from a kernel thread. Thread becomes zombie which resources will
 * eventually be recycled. */
noreturn void thread_exit(int exitcode);

/* Debugging utility that prints out the summary of all_threads contents. */
void thread_dump_all();

/* Returns the thread matching the given ID, or null if none found. */
thread_t *thread_get_by_tid(tid_t id);

/* Joins the specified thread, effectively waiting until it exits. */
void thread_join(thread_t *td);

/* Reaps zombie threads. You do not need to call this function on your own,
   reaping will automatically take place when convenient. The reason this
   function is exposed is because some tests need to explicitly wait until
   threads are reaped before they can verify test success. */
void thread_reap();

#endif /* _SYS_THREAD_H_ */
