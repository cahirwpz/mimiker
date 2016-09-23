#include <stdc.h>
#include <callout.h>

#define CALLOUT_BUCKETS 5

#define callout_set_active(c) \
  ((c)->c_flags |= CALLOUT_ACTIVE)
#define callout_clear_active(c) \
  ((c)->c_flags &= ~CALLOUT_ACTIVE)

#define callout_set_pending(c) \
  ((c)->c_flags |= CALLOUT_PENDING)
#define callout_clear_pending(c) \
  ((c)->c_flags &= ~CALLOUT_PENDING)

/*
  Every event is inside one of CALLOUT_BUCKETS buckets.
  The buckets is a cyclic list, but we implement it as an array,
  allowing us to access random elements in constant time.
*/

typedef struct callout_head callout_head_t;
TAILQ_HEAD(callout_head, callout);

static struct {
  callout_head_t heads[CALLOUT_BUCKETS];
  realtime_t last;
} ci;

void callout_init() {
  bzero(&ci, sizeof(ci));

  for (int i = 0; i < CALLOUT_BUCKETS; i++)
    TAILQ_INIT(&ci.heads[i]);
}

void callout_setup(callout_t *handle, realtime_t time, timeout_t fn, void *arg)
{
  int index = time % CALLOUT_BUCKETS;

  bzero(handle, sizeof(callout_t));
  handle->c_time = time;
  handle->c_func = fn;
  handle->c_arg = arg;
  handle->c_index = index;
  callout_set_pending(handle);

  log("Add callout {%p} with wakeup at %ld.", handle, (size_t)handle->c_time);
  TAILQ_INSERT_TAIL(&ci.heads[index], handle, c_link);
}

void callout_stop(callout_t *handle) {
  log("Remove callout {%p} at %ld.", handle, (size_t)handle->c_time);
  TAILQ_REMOVE(&ci.heads[handle->c_index], handle, c_link);
}

/*
 * Handle all timeouted callouts from queues between last position and current
 * position.
*/
void callout_process(realtime_t time) {
  int now = time % CALLOUT_BUCKETS;
  bool done = false;

  while (!done) {
    int last = ci.last++ % CALLOUT_BUCKETS;

    if (last == now)
      done = true;

    callout_head_t *head = &ci.heads[last];
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
  }
}
