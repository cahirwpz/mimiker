#define KL_LOG KL_CALLOUT
#include <klog.h>
#include <stdc.h>
#include <callout.h>
#include <spinlock.h>
#include <sysinit.h>
#include <sleepq.h>
#include <thread.h>
#include <sched.h>
#include <interrupt.h>

/* Note: If the difference in time between ticks is greater than the number of
   buckets, some callouts may be called out-of-order! */
#define CALLOUT_BUCKETS 64

#define callout_is_active(c) ((c)->c_flags & CALLOUT_ACTIVE)
#define callout_set_active(c) ((c)->c_flags |= CALLOUT_ACTIVE)
#define callout_clear_active(c) ((c)->c_flags &= ~CALLOUT_ACTIVE)

#define callout_is_pending(c) ((c)->c_flags & CALLOUT_PENDING)
#define callout_set_pending(c) ((c)->c_flags |= CALLOUT_PENDING)
#define callout_clear_pending(c) ((c)->c_flags &= ~CALLOUT_PENDING)

/*
  Every event is inside one of CALLOUT_BUCKETS buckets.
  The buckets is a cyclic list, but we implement it as an array,
  allowing us to access random elements in constant time.
*/

typedef TAILQ_HEAD(callout_list, callout) callout_list_t;

static struct {
  callout_list_t heads[CALLOUT_BUCKETS];
  /* Stores the value of the argument callout_process was previously
     called with. All callouts up to this timestamp have already been
     processed. */
  systime_t last;
  spin_t lock;
} ci;

static inline callout_list_t *ci_list(int i) {
  return &ci.heads[i];
}

callout_list_t shared;

static void callout_thread(void *arg) {
  while (true) {
    callout_t *first;

    WITH_INTR_DISABLED {
      while (TAILQ_EMPTY(&shared)) {
        sleepq_wait(&shared, NULL);
      }

      first = TAILQ_FIRST(&shared);
      TAILQ_REMOVE(&shared, first, c_link);
    }
    
    assert(first->c_func != NULL);
    first->c_func(first->c_arg);
    /* Wake threads that wait for execution of this callout in function
     * callout_drain. */
    sleepq_broadcast(first);
    callout_clear_active(first);
  }
}

static void callout_init(void) {
  bzero(&ci, sizeof(ci));

  ci.lock = SPIN_INITIALIZER(0);

  TAILQ_INIT(&shared);

  thread_t *td =
    thread_create("callout-thread", callout_thread, NULL, prio_kthread(0));
  sched_add(td);

  for (int i = 0; i < CALLOUT_BUCKETS; i++)
    TAILQ_INIT(ci_list(i));
}

static void _callout_setup(callout_t *handle, systime_t time, timeout_t fn,
                           void *arg) {
  assert(spin_owned(&ci.lock));
  assert(!callout_is_pending(handle));

  int index = time % CALLOUT_BUCKETS;

  bzero(handle, sizeof(callout_t));
  handle->c_time = time;
  handle->c_func = fn;
  handle->c_arg = arg;
  handle->c_index = index;
  callout_set_pending(handle);

  klog("Add callout {%p} with wakeup at %ld.", handle, handle->c_time);
  TAILQ_INSERT_TAIL(ci_list(index), handle, c_link);
}

void callout_setup(callout_t *handle, systime_t time, timeout_t fn, void *arg) {
  SCOPED_SPIN_LOCK(&ci.lock);

  _callout_setup(handle, time, fn, arg);
}

void callout_setup_relative(callout_t *handle, systime_t time, timeout_t fn,
                            void *arg) {
  SCOPED_SPIN_LOCK(&ci.lock);

  systime_t now = getsystime();
  _callout_setup(handle, now + time, fn, arg);
}

void callout_stop(callout_t *handle) {
  SCOPED_SPIN_LOCK(&ci.lock);

  klog("Remove callout {%p} at %ld.", handle, handle->c_time);

  if (callout_is_pending(handle)) {
    callout_clear_pending(handle);
    TAILQ_REMOVE(ci_list(handle->c_index), handle, c_link);
  }
}

/*
 * Handle all timeouted callouts from queues between last position and current
 * position.
 */
void callout_process(systime_t time) {
  unsigned int last_bucket;
  unsigned int current_bucket = ci.last % CALLOUT_BUCKETS;

  if (time - ci.last > CALLOUT_BUCKETS) {
    /* Process all buckets */
    last_bucket = (ci.last - 1) % CALLOUT_BUCKETS;
  } else {
    /* Process only buckets in time range ci.last to time */
    last_bucket = time % CALLOUT_BUCKETS;
  }

  /* We are in kernel's bottom half. */
  assert(intr_disabled());

  while (true) {
    callout_list_t *head = ci_list(current_bucket);
    callout_t *elem, *next;

    /* Detach triggered callouts from the queue. */
    TAILQ_FOREACH_SAFE(elem, head, c_link, next) {
      if (elem->c_time <= time) {
        callout_set_active(elem);
        callout_clear_pending(elem);
        TAILQ_REMOVE(head, elem, c_link);
        TAILQ_INSERT_TAIL(&shared, elem, c_link);
      }
    }

    if (current_bucket == last_bucket)
      break;

    current_bucket = (current_bucket + 1) % CALLOUT_BUCKETS;
  }

  ci.last = time;

  if (!TAILQ_EMPTY(&shared)) {
    sleepq_signal(&shared);
  }
}

bool callout_drain(callout_t *handle) {
  /* A callout may be in active state only in callout_process,
   * which is called in bottom half (with interrupts disabled),
   * so we can't notice it. */
  assert(!callout_is_active(handle));

  WITH_INTR_DISABLED {
    if (callout_is_pending(handle)) {
      sleepq_wait(handle, NULL);
      return true;
    }
  }

  return false;
}

SYSINIT_ADD(callout, callout_init, DEPS("sched"));
