#define KL_LOG KL_THREAD
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/pool.h>
#include <sys/malloc.h>
#include <sys/thread.h>
#include <sys/pcpu.h>
#include <sys/sched.h>
#include <sys/sleepq.h>
#include <sys/filedesc.h>
#include <sys/turnstile.h>
#include <sys/kmem.h>

static POOL_DEFINE(P_THREAD, "thread", sizeof(thread_t));

typedef TAILQ_HEAD(, thread) thread_list_t;

static mtx_t *threads_lock = &MTX_INITIALIZER(0);
static thread_list_t all_threads = TAILQ_HEAD_INITIALIZER(all_threads);
static thread_list_t zombie_threads = TAILQ_HEAD_INITIALIZER(zombie_threads);

/* FTTB such a primitive method of creating new TIDs will do. */
static tid_t make_tid(void) {
  static volatile tid_t tid = 0;
  SCOPED_NO_PREEMPTION();
  return tid++;
}

void thread_reap(void) {
  thread_list_t zombies;

  WITH_MTX_LOCK (threads_lock) {
    zombies = zombie_threads;
    TAILQ_INIT(&zombie_threads);
  }

  thread_t *td, *next;
  TAILQ_FOREACH_SAFE (td, &zombies, td_zombieq, next)
    thread_delete(td);
}

static void thread_init(thread_t *td, prio_t prio) {
  td->td_tid = make_tid();
  td->td_state = TDS_INACTIVE;

  td->td_prio = prio;
  td->td_base_prio = prio;

  td->td_spin = SPIN_INITIALIZER(0);
  td->td_lock = MTX_INITIALIZER(0);
  cv_init(&td->td_waitcv, "thread waiters");
  LIST_INIT(&td->td_contested);
  bzero(&td->td_slpcallout, sizeof(callout_t));

  /* From now on, you must use locks on new thread structure. */
  WITH_MTX_LOCK (threads_lock)
    TAILQ_INSERT_TAIL(&all_threads, td, td_all);
}

static alignas(PAGESIZE) uint8_t _stack0[PAGESIZE];
static thread_t _thread0[1];

/* Creates Thread Zero - first thread in the system. */
void init_thread0(void) {
  thread_t *td = _thread0;
  td->td_name = "kernel-main";
  kstack_init(&td->td_kstack, _stack0, sizeof(_stack0));

  /* Thread Zero is initially running with interrupts disabled! */
  td->td_idnest = 1;
  td->td_state = TDS_RUNNING;
  PCPU_SET(curthread, td);

  /* Note that initially Thread Zero has no turnstile or sleepqueue attached.
   * Corresponding subsystems are started before scheduler. We can add missing
   * pieces to first thread in turnstile & sleepqueue init procedures. */
  thread_init(td, prio_uthread(255));
}

thread_t *thread_create(const char *name, void (*fn)(void *), void *arg,
                        prio_t prio) {
  /* Firstly recycle some threads to free up memory. */
  thread_reap();

  thread_t *td = pool_alloc(P_THREAD, M_ZERO);
  thread_init(td, prio);

  td->td_name = kstrndup(M_STR, name, TD_NAME_MAX);
  kstack_init(&td->td_kstack, kmem_alloc(PAGESIZE, M_ZERO), PAGESIZE);

  td->td_sleepqueue = sleepq_alloc();
  td->td_turnstile = turnstile_alloc();

  thread_entry_setup(td, fn, arg);

  klog("Thread %ld {%p} has been created", td->td_tid, td);

  return td;
}

void thread_delete(thread_t *td) {
  assert(td_is_dead(td));
  assert(td->td_sleepqueue != NULL);
  assert(td->td_turnstile != NULL);

  klog("Freeing up thread %ld {%p}", td->td_tid, td);

  WITH_MTX_LOCK (threads_lock)
    TAILQ_REMOVE(&all_threads, td, td_all);

  kmem_free(td->td_kstack.stk_base, PAGESIZE);

  callout_drain(&td->td_slpcallout);
  sleepq_destroy(td->td_sleepqueue);
  turnstile_destroy(td->td_turnstile);
  kfree(M_STR, td->td_name);
  pool_free(P_THREAD, td);
}

thread_t *thread_self(void) {
  return PCPU_GET(curthread);
}

/* For now this is only a stub
 * NOTE: this procedure must NOT access the thread's process state
 * in ANY way, as the process might have already been reaped. */
__noreturn void thread_exit(void) {
  thread_t *td = thread_self();

  klog("Thread %ld {%p} has finished", td->td_tid, td);

  /* Thread must not exit while having interrupts disabled! However, we can't
   * use assert here, because assert also calls thread_exit. Thus, in case this
   * condition is not met, we'll log the problem and panic! */
  if (intr_disabled())
    panic("ERROR: Thread must not exit when interrupts are disabled!");

  /*
   * Preemption must be disabled for the code below, otherwise the thread may be
   * kicked out of processor without possiblity to return and finish the job.
   *
   * The thread may get switched out as a result of acquiring locks but it
   * hardly matters as we start performing actions with both locks acquired.
   */
  preempt_disable();

  WITH_MTX_LOCK (threads_lock) {
    mtx_lock(&td->td_lock); /* force threads_lock >> thread_t::td_lock order */
    TAILQ_INSERT_TAIL(&zombie_threads, td, td_zombieq);
  }

  cv_broadcast(&td->td_waitcv);
  mtx_unlock(&td->td_lock);

  WITH_SPIN_LOCK (&td->td_spin) {
    td->td_state = TDS_DEAD;
    sched_switch();
  }

  panic("Thread %u tried to ressurect", td->td_tid);
}

void thread_join(thread_t *otd) {
  thread_t *td = thread_self();

  SCOPED_MTX_LOCK(&otd->td_lock);

  klog("Join %ld {%p} with %ld {%p}", td->td_tid, td, otd->td_tid, otd);

  while (!td_is_dead(otd))
    cv_wait(&otd->td_waitcv, &otd->td_lock);
}

void thread_yield(void) {
  thread_t *td = thread_self();

  WITH_SPIN_LOCK (&td->td_spin) {
    td->td_state = TDS_READY;
    sched_switch();
  }
}

/* It would be better to have a hash-map from tid_t to thread_t,
 * but using a list is sufficient for now. */
thread_t *thread_find(tid_t id) {
  SCOPED_MTX_LOCK(threads_lock);

  thread_t *td;
  TAILQ_FOREACH (td, &all_threads, td_all) {
    mtx_lock(&td->td_lock);
    if (td->td_tid == id)
      return td;
    mtx_unlock(&td->td_lock);
  }
  return NULL;
}
