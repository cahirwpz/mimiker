#ifndef _TIMER_H_
#define _TIMER_H_

#include <queue.h>
#include <clock.h>

typedef struct timer_event timer_event_t;

struct timer_event {
  TAILQ_ENTRY(timer_event) tev_link; /*  entry on list of all time events */
  timeval_t tev_when;                /*  absolute time when this event should
                                          be handled */
  void (*tev_func)(timer_event_t *self); /*  callback function called in
                                             interrupt context */
  void *tev_data;                        /*  auxiliary data */
};

int cpu_timer_add_event(timer_event_t *tev);
int cpu_timer_remove_event(timer_event_t *tev);

/*  Processes core timer interrupts. */
void cpu_timer_intr(void *arg);

#endif /*  !_TIMER_H_ */
