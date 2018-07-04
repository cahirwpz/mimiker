#define KL_LOG KL_THREAD
#include <klog.h>
#include <malloc.h>
#include <physmem.h>
#include <thread.h>
#include <context.h>
#include <interrupt.h>
#include <pcpu.h>
#include <sched.h>
#include <filedesc.h>
#include <mips/exc.h>

static MALLOC_DEFINE(M_THREAD, "thread", 1, 2);

typedef TAILQ_HEAD(, thread) thread_list_t;

static mtx_t *threads_lock = &MTX_INITIALIZER(MTX_DEF);
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

  thread_t *td;
  TAILQ_FOREACH (td, &zombies, td_zombieq)
    thread_delete(td);
}

extern noreturn void thread_exit(void);
extern noreturn void kern_exc_leave(void);

void thread_entry_setup(thread_t *td, entry_fn_t target, void *arg) {
  stack_t *stk = &td->td_kstack;

  /* For threads that are allowed to enter user-space (for now - all of them),
   * full exception frame has to be allocated at the bottom of kernel stack.
   * Just under it there's a kernel exception frame (cpu part of full one) that
   * is used to enter kernel thread for the first time. */
  exc_frame_t *uframe = stack_alloc_s(stack_bottom(stk), exc_frame_t);
  exc_frame_t *kframe = stack_alloc_s(uframe, cpu_exc_frame_t);

  td->td_uframe = uframe;
  td->td_kframe = kframe;

  /* Initialize registers just for ctx_switch to work correctly. */
  ctx_init(&td->td_kctx, kern_exc_leave, kframe);

  /* This is the context that kern_exc_leave will restore. */
  exc_frame_init(kframe, target, uframe, EF_KERNEL);
  exc_frame_setup_call(kframe, thread_exit, (long)arg, 0);
}

thread_t *thread_create(const char *name, void (*fn)(void *), void *arg) {
  /* Firstly recycle some threads to free up memory. */
  thread_reap();

  thread_t *td = kmalloc(M_THREAD, sizeof(thread_t), M_ZERO);

  td->td_sleepqueue = sleepq_alloc();
  td->td_turnstile = turnstile_alloc();
  td->td_name = kstrndup(M_THREAD, name, TD_NAME_MAX);
  td->td_tid = make_tid();
  td->td_kstack_obj = pm_alloc(1);
  td->td_kstack.stk_base = (void *)PG_VADDR_START(td->td_kstack_obj);
  td->td_kstack.stk_size = PAGESIZE;
  td->td_state = TDS_INACTIVE;

  spin_init(td->td_spin);
  mtx_init(&td->td_lock, MTX_RECURSE);
  cv_init(&td->td_waitcv, "thread waiters");
  LIST_INIT(&td->td_contested);

  thread_entry_setup(td, fn, arg);

  /* From now on, you must use locks on new thread structure. */
  WITH_MTX_LOCK (threads_lock)
    TAILQ_INSERT_TAIL(&all_threads, td, td_all);

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

  pm_free(td->td_kstack_obj);

  sleepq_destroy(td->td_sleepqueue);
  turnstile_destroy(td->td_turnstile);
  kfree(M_THREAD, td->td_name);
  kfree(M_THREAD, td);
}

thread_t *thread_self(void) {
  return PCPU_GET(curthread);
}

/* For now this is only a stub */
noreturn void thread_exit(void) {
  thread_t *td = thread_self();

  klog("Thread %ld {%p} has finished", td->td_tid, td);

  /* Thread must not exit while having interrupts disabled! However, we can't
   * use assert here, because assert also calls thread_exit. Thus, in case this
   * condition is not met, we'll log the problem and panic! */
  if (td->td_idnest > 0)
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

  WITH_SPINLOCK(td->td_spin) {
    td->td_state = TDS_DEAD;
    sched_switch();
  }

  panic("Thread %ld tried to ressurect", td->td_tid);
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

  WITH_SPINLOCK(td->td_spin) {
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
