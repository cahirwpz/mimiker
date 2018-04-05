#ifndef _SYS_THREAD_H_
#define _SYS_THREAD_H_

#include <common.h>
#include <queue.h>
#include <context.h>
#include <exception.h>
#include <sleepq.h>
#include <turnstile.h>
#include <mutex.h>
#include <condvar.h>
#include <time.h>
#include <signal.h>
#include <spinlock.h>

/*! \file thread.h */

typedef uint8_t td_prio_t;
typedef struct vm_page vm_page_t;
typedef struct vm_map vm_map_t;
typedef struct fdtab fdtab_t;
typedef struct proc proc_t;

#define TD_NAME_MAX 32

/*! \brief Possible thread states.
 *
 * Possible state transitions look as follows:
 *  - INACTIVE -> READY (parent thread)
 *  - READY -> RUNNING (dispatcher)
 *  - RUNNING -> READY (dispatcher, self)
 *  - RUNNING -> SLEEPING (self)
 *  - SLEEPING -> READY (interrupts, other threads)
 *  - * -> DEAD (other threads or self)
 */
typedef enum {
  /*!< thread was created but it has not been run so far */
  TDS_INACTIVE,
  /*!< thread was put onto a run queue and it's ready to be dispatched */
  TDS_READY,
  /*!< thread was removed from a run queue and is being run at the moment */
  TDS_RUNNING,
  /*!< thread is waiting on a resource and it has been put on a sleep queue */
  TDS_SLEEPING,
  /*!< thread finished or was terminated by the kernel and awaits recycling */
  TDS_DEAD
} thread_state_t;

#define TDF_SLICEEND 0x00000001   /* run out of time slice */
#define TDF_NEEDSWITCH 0x00000002 /* must switch on next opportunity */
#define TDF_NEEDSIGCHK 0x00000004 /* signals were posted for delivery */
#define TDF_NEEDLOCK 0x00000008   /* acquire td_spin on context switch */

/*! \brief Thread structure
 *
 * Field markings and the corresponding locks:
 *  - a: threads_lock
 *  - t: thread_t::td_lock
 *  - @: read-only access
 *  - !: thread_t::td_spin
 *  - ~: always safe to access
 *
 * Locking order:
 *  threads_lock >> thread_t::td_lock
 */
typedef struct thread {
  /* locks */
  spinlock_t td_spin[1]; /*!< (~) synchronizes top & bottom halves */
  mtx_t td_lock;         /*!< (~) protects most fields in this structure */
  condvar_t td_waitcv;   /*!< (t) for thread_join */
  /* linked lists */
  TAILQ_ENTRY(thread) td_all;     /* a link on all threads list */
  TAILQ_ENTRY(thread) td_runq;    /* a link on run queue */
  TAILQ_ENTRY(thread) td_sleepq;  /* a link on sleep queue */
  TAILQ_ENTRY(thread) td_zombieq; /* a link on zombie queue */
  TAILQ_ENTRY(thread) td_procq;   /* a link on process threads queue */
  /* Properties */
  proc_t *td_proc; /*!< (t) parent process (NULL for kernel threads) */
  char *td_name;   /*!< (@) name of thread */
  tid_t td_tid;    /*!< (@) thread identifier */
  /* thread state */
  thread_state_t td_state; /*!< (!) thread state */
  uint32_t td_flags;       /*!< (!) TDF_* flags */
  /* thread context */
  volatile unsigned td_idnest; /*!< (?) interrupt disable nest level */
  volatile unsigned td_pdnest; /*!< (?) preemption disable nest level */
  exc_frame_t td_uctx;         /* user context (always exception) */
  fpu_ctx_t td_uctx_fpu;       /* user FPU context (always exception) */
  exc_frame_t *td_kframe;      /* kernel context (last exception frame) */
  ctx_t td_kctx;               /* kernel context (switch) */
  intptr_t td_onfault;         /* program counter for copyin/copyout faults */
  vm_page_t *td_kstack_obj;
  stack_t td_kstack;
  /* waiting channel */
  sleepq_t *td_sleepqueue;
  turnstile_t *td_turnstile;
  void *td_wchan;
  const void *td_waitpt; /*!< a point where program waits */
  /* scheduler part */
  td_prio_t td_prio;
  int td_slice;
  /* thread statistics */
  timeval_t td_rtime;        /*!< time spent running */
  timeval_t td_last_rtime;   /*!< time of last switch to running state */
  timeval_t td_slptime;      /*!< time spent sleeping */
  timeval_t td_last_slptime; /*!< time of last switch to sleep state */
  unsigned td_nctxsw;        /*!< total number of context switches */
  /* signal handling */
  sigset_t td_sigpend; /* Pending signals for this thread. */
  /* TODO: Signal mask, sigsuspend. */
} thread_t;

thread_t *thread_self(void);
thread_t *thread_create(const char *name, void (*fn)(void *), void *arg);
void thread_delete(thread_t *td);

/*! \brief Exit from a thread.
 *
 * Thread becomes zombie which resources will eventually be recycled.
 */
noreturn void thread_exit(void);

/*! \brief Switch voluntarily to another thread. */
void thread_yield(void);

/*! \brief Find thread with matching \a id.
 *
 * \return Returned thread is locked.
 */
thread_t *thread_find(tid_t id);

/* Joins the specified thread, effectively waiting until it exits. */
void thread_join(thread_t *td);

/*! \brief Recycles dead threads.
 *
 * You do not need to call this function on your own, reaping will automatically
 * take place when convenient. The reason this function is exposed is because
 * some tests need to explicitly wait until threads are reaped before they can
 * verify test success. */
void thread_reap(void);

#endif /* !_SYS_THREAD_H_ */
