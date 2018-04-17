#ifndef _SYS_TIMER_H_
#define _SYS_TIMER_H_

#include <queue.h>
#include <time.h>

typedef struct timer timer_t;
typedef TAILQ_HEAD(, timer) timer_list_t;

typedef bintime_t (*tm_gettime_t)(timer_t *tm);
typedef int (*tm_start_t)(timer_t *tm, unsigned flags, const bintime_t value);
typedef int (*tm_stop_t)(timer_t *tm);
typedef void (*tm_event_cb_t)(timer_t *tm, void *arg);

#define TMF_ONESHOT 0x0001
#define TMF_PERIODIC 0x0002
#define TMF_TYPEMASK 0x0003
#define TMF_ACTIVE 0x1000
#define TMF_INITIALIZED 0x2000
#define TMF_ALLOCATED 0x4000
#define TMF_REGISTERED 0x8000

typedef struct timer {
  TAILQ_ENTRY(timer) tm_link;
  const char *tm_name;
  unsigned tm_flags;
  bintime_t tm_min_period;
  bintime_t tm_max_period;
  tm_gettime_t tm_gettime;
  tm_start_t tm_start;
  tm_stop_t tm_stop;
  tm_event_cb_t tm_event_cb;
  void *tm_arg;
  void *tm_priv; /*!< private data (usually pointer to device_t) */
} timer_t;

int tm_register(timer_t *tm);
int tm_deregister(timer_t *tm);
timer_t *tm_alloc(const char *name, unsigned flags);
int tm_init(timer_t *tm, tm_event_cb_t event, void *arg);
int tm_free(timer_t *tm);

bintime_t tm_gettime(timer_t *tm);
int tm_start(timer_t *tm, unsigned flags, const bintime_t value);
int tm_stop(timer_t *tm);
void tm_trigger(timer_t *tm);

#endif /* !_SYS_TIMER_H_ */
