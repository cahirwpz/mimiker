#define KL_LOG KL_CALLOUT
#include <klog.h>
#include <stdc.h>
#include <callout.h>
#include <sysinit.h>

/* Note: If the difference in time between ticks is greater than the number of
   buckets, some callouts may be called out-of-order! */
#define CALLOUT_BUCKETS 64

#define callout_set_active(c) ((c)->c_flags |= CALLOUT_ACTIVE)
#define callout_clear_active(c) ((c)->c_flags &= ~CALLOUT_ACTIVE)

#define callout_set_pending(c) ((c)->c_flags |= CALLOUT_PENDING)
#define callout_clear_pending(c) ((c)->c_flags &= ~CALLOUT_PENDING)

/*
  Every event is inside one of CALLOUT_BUCKETS buckets.
  The buckets is a cyclic list, but we implement it as an array,
  allowing us to access random elements in constant time.
*/

typedef struct callout_head callout_head_t;
TAILQ_HEAD(callout_head, callout);

static struct {
  callout_head_t heads[CALLOUT_BUCKETS];
  /* Stores the value of the argument callout_process was previously
     called with. All callouts up to this timestamp have already been
     processed. */
  realtime_t last;
} ci;

static void callout_init() {
  bzero(&ci, sizeof(ci));

  for (int i = 0; i < CALLOUT_BUCKETS; i++)
    TAILQ_INIT(&ci.heads[i]);
}

void callout_setup(callout_t *handle, realtime_t time, timeout_t fn,
                   void *arg) {
  int index = time % CALLOUT_BUCKETS;

  bzero(handle, sizeof(callout_t));
  handle->c_time = time;
  handle->c_func = fn;
  handle->c_arg = arg;
  handle->c_index = index;
  callout_set_pending(handle);

  klog("Add callout {%p} with wakeup at %lld.", handle, handle->c_time);
  TAILQ_INSERT_TAIL(&ci.heads[index], handle, c_link);
}

void callout_setup_relative(callout_t *handle, realtime_t time, timeout_t fn,
                            void *arg) {
  callout_setup(handle, time + ci.last, fn, arg);
}

void callout_stop(callout_t *handle) {
  klog("Remove callout {%p} at %lld.", handle, handle->c_time);
  TAILQ_REMOVE(&ci.heads[handle->c_index], handle, c_link);
}

/*
 * Handle all timeouted callouts from queues between last position and current
 * position.
*/
void callout_process(realtime_t time) {
  unsigned int last_bucket;
  unsigned int current_bucket = ci.last % CALLOUT_BUCKETS;
  if (time - ci.last > CALLOUT_BUCKETS) {
    /* Process all buckets */
    last_bucket = (ci.last - 1) % CALLOUT_BUCKETS;
  } else {
    /* Process only buckets in time range ci.last to time */
    last_bucket = time % CALLOUT_BUCKETS;
  }

  while (1) {
    callout_head_t *head = &ci.heads[current_bucket];
    callout_t *elem = TAILQ_FIRST(head);
    callout_t *next;

    while (elem) {
      next = TAILQ_NEXT(elem, c_link);

      if (elem->c_time <= time) {
        callout_set_active(elem);
        callout_clear_pending(elem);

        TAILQ_REMOVE(head, elem, c_link);
        elem->c_func(elem->c_arg);

        callout_clear_active(elem);
      }

      elem = next;
    }
    if (current_bucket == last_bucket)
      break;

    current_bucket = (current_bucket + 1) % CALLOUT_BUCKETS;
  }

  ci.last = time;
}

SYSINIT_ADD(callout, callout_init, NODEPS);
