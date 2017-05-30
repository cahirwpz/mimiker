#ifndef _SYS_TIMER_H_
#define _SYS_TIMER_H_

#include <queue.h>
#include <time.h>

typedef struct timer_event timer_event_t;
typedef TAILQ_HEAD(, timer_event) timer_event_list_t;

struct timer_event {
  /* entry on list of all time events */
  TAILQ_ENTRY(timer_event) tev_link;
  /* absolute time when this event should be handled */
  timeval_t tev_when;
  /* callback function called in interrupt context */
  void (*tev_func)(timer_event_t *self);
  /* auxiliary data */
  void *tev_data;
};

void cpu_timer_add_event(timer_event_t *tev);
void cpu_timer_remove_event(timer_event_t *tev);

#endif /* !_SYS_TIMER_H_ */
