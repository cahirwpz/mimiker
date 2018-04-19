#define KL_LOG KL_TIME
#include <timer.h>
#include <mutex.h>
#include <errno.h>
#include <klog.h>
#include <interrupt.h>

static mtx_t timers_mtx = MTX_INITIALIZER(MTX_DEF);
static timer_list_t timers = TAILQ_HEAD_INITIALIZER(timers);

#define TMF_ACTIVE 0x1000
#define TMF_INITIALIZED 0x2000
#define TMF_ALLOCATED 0x4000
#define TMF_REGISTERED 0x8000

#define is_active(tm) ((tm)->tm_flags & TMF_ACTIVE)
#define is_initialized(tm) ((tm)->tm_flags & TMF_INITIALIZED)
#define is_allocated(tm) ((tm)->tm_flags & TMF_ALLOCATED)
#define is_registered(tm) ((tm)->tm_flags & TMF_REGISTERED)

int tm_register(timer_t *tm) {
  if (is_registered(tm))
    return EBUSY;

  WITH_MTX_LOCK (&timers_mtx) {
    TAILQ_INSERT_TAIL(&timers, tm, tm_link);
    tm->tm_flags &= TMF_TYPEMASK;
    tm->tm_flags |= TMF_REGISTERED;
  }

  klog("Registered '%s' timer.", tm->tm_name);
  return 0;
}

int tm_deregister(timer_t *tm) {
  assert(is_registered(tm));

  if (is_allocated(tm))
    return EBUSY;

  WITH_MTX_LOCK (&timers_mtx) {
    TAILQ_REMOVE(&timers, tm, tm_link);
    tm->tm_flags &= TMF_TYPEMASK;
  }

  klog("Unregistered '%s' timer.", tm->tm_name);
  return 0;
}

timer_t *tm_alloc(const char *name, unsigned flags) {
  timer_t *tm = NULL;

  WITH_MTX_LOCK (&timers_mtx) {
    TAILQ_FOREACH (tm, &timers, tm_link) {
      if (is_allocated(tm))
        continue;
      if (tm->tm_flags & flags)
        break;
    }
    tm->tm_flags |= TMF_ALLOCATED;
  }
  return tm;
}

int tm_init(timer_t *tm, tm_event_cb_t event, void *arg) {
  assert(is_allocated(tm));

  if (is_active(tm))
    return EBUSY;

  tm->tm_flags |= TMF_INITIALIZED;
  tm->tm_event_cb = event;
  tm->tm_arg = arg;
  return 0;
}

int tm_start(timer_t *tm, unsigned flags, const bintime_t value) {
  assert(is_initialized(tm));

  if (is_active(tm))
    return EBUSY;
  if (((tm->tm_flags & flags) & TMF_TYPEMASK) == 0)
    return ENODEV;
  if (flags & TMF_PERIODIC) {
    if (bintime_cmp(value, tm->tm_min_period, <) ||
        bintime_cmp(value, tm->tm_max_period, >))
      return EINVAL;
  }

  int retval = tm->tm_start(tm, flags, value);
  if (retval == 0)
    tm->tm_flags |= TMF_ACTIVE;
  return retval;
}

int tm_stop(timer_t *tm) {
  assert(is_initialized(tm));

  if (!is_active(tm))
    return 0;

  int retval = tm->tm_stop(tm);
  if (retval == 0)
    tm->tm_flags &= ~TMF_ACTIVE;
  return retval;
}

void tm_trigger(timer_t *tm) {
  assert(is_initialized(tm));
  assert(intr_disabled());

  tm->tm_event_cb(tm, tm->tm_arg);
}
