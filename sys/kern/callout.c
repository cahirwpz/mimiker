#define KL_LOG KL_CALLOUT
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/callout.h>
#include <sys/spinlock.h>
#include <sys/sleepq.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <sys/interrupt.h>

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

static callout_list_t delegated;

static void _callout_schedule(callout_t *handle) {
  assert(spin_owned(&ci.lock));

  callout_set_pending(handle);

  klog("Add callout {%p} with wakeup at %ld.", handle, handle->c_time);
  TAILQ_INSERT_TAIL(ci_list(handle->c_index), handle, c_link);
}

static void callout_thread(void *arg) {
  while (true) {
    callout_t *elem;

    WITH_INTR_DISABLED {
      while (TAILQ_EMPTY(&delegated)) {
        sleepq_wait(&delegated, NULL);
      }

      elem = TAILQ_FIRST(&delegated);
      TAILQ_REMOVE(&delegated, elem, c_link);
    }

    assert(callout_is_active(elem));
    assert(!callout_is_pending(elem));

    /* Execute callout's function. */
    elem->c_func(elem->c_arg);

    WITH_SPIN_LOCK (&ci.lock) {
      callout_clear_active(elem);
      if (elem->c_interval) {
        /* Periodic callout: reschedule. */
        elem->c_time += elem->c_interval;
        _callout_schedule(elem);
      } else {
        /* Wake threads that wait for execution of this callout in
         * callout_drain. */
        sleepq_broadcast(elem);
      }
    }
  }
}

void init_callout(void) {
  bzero(&ci, sizeof(ci));

  ci.lock = SPIN_INITIALIZER(0);

  for (int i = 0; i < CALLOUT_BUCKETS; i++)
    TAILQ_INIT(ci_list(i));

  TAILQ_INIT(&delegated);

  thread_t *td =
    thread_create("callout", callout_thread, NULL, prio_kthread(0));
  sched_add(td);
}

static void _callout_setup(callout_t *handle, systime_t time,
                           systime_t interval, timeout_t fn, void *arg) {
  assert(spin_owned(&ci.lock));
  assert(!callout_is_pending(handle));
  assert(!callout_is_active(handle));

  int index = time % CALLOUT_BUCKETS;

  bzero(handle, sizeof(callout_t));
  handle->c_time = time;
  handle->c_interval = interval;
  handle->c_func = fn;
  handle->c_arg = arg;
  handle->c_index = index;

  _callout_schedule(handle);
}

void callout_setup(callout_t *handle, systime_t time, timeout_t fn, void *arg) {
  SCOPED_SPIN_LOCK(&ci.lock);

  _callout_setup(handle, time, 0, fn, arg);
}

void callout_setup_relative(callout_t *handle, systime_t time, timeout_t fn,
                            void *arg) {
  SCOPED_SPIN_LOCK(&ci.lock);

  systime_t now = getsystime();
  _callout_setup(handle, now + time, 0, fn, arg);
}

void callout_setup_periodic(callout_t *handle, systime_t time,
                            systime_t interval, timeout_t fn, void *arg) {
  SCOPED_SPIN_LOCK(&ci.lock);

  _callout_setup(handle, time, interval, fn, arg);
}

void callout_setup_relative_periodic(callout_t *handle, systime_t time,
                                     systime_t interval, timeout_t fn,
                                     void *arg) {
  SCOPED_SPIN_LOCK(&ci.lock);

  systime_t now = getsystime();
  _callout_setup(handle, now + time, interval, fn, arg);
}

bool callout_stop(callout_t *handle) {
  SCOPED_SPIN_LOCK(&ci.lock);

  klog("Remove callout {%p} at %ld.", handle, handle->c_time);

  handle->c_interval = 0;

  if (callout_is_pending(handle)) {
    callout_clear_pending(handle);
    TAILQ_REMOVE(ci_list(handle->c_index), handle, c_link);
    return true;
  }

  return false;
}

/*
 * Process all timeouted callouts from queues between last position and
 * current position and delegate them to callout thread.
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

    /* Detach triggered callouts from ci_list queue. */
    TAILQ_FOREACH_SAFE (elem, head, c_link, next) {
      if (elem->c_time <= time) {
        callout_set_active(elem);
        callout_clear_pending(elem);
        TAILQ_REMOVE(head, elem, c_link);
        /* Attach elem to callout thread's queue. */
        TAILQ_INSERT_TAIL(&delegated, elem, c_link);
      }
    }

    if (current_bucket == last_bucket)
      break;

    current_bucket = (current_bucket + 1) % CALLOUT_BUCKETS;
  }

  ci.last = time;

  /* Wake callout thread. */
  if (!TAILQ_EMPTY(&delegated)) {
    sleepq_signal(&delegated);
  }
}

bool callout_drain(callout_t *handle) {
  /* Disallow draining periodic callouts that haven't been stopped. */
  assert(handle->c_interval == 0);
  WITH_INTR_DISABLED {
    if (callout_is_pending(handle) || callout_is_active(handle)) {
      sleepq_wait(handle, NULL);
      return true;
    }
  }

  return false;
}
