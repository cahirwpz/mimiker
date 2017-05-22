#ifndef _MIPS_CLOCK_H_
#define _MIPS_CLOCK_H_

#include <queue.h>
#include <clock.h>

typedef struct timer_event timer_event_t;

struct timer_event {
      TAILQ_ENTRY(timer_event) tev_link;     /*  entry on list of all time events */
       timeval_t tev_alarm;                   /*  absolute time when this event should
                                                  be handled */
        void (*tev_func)(timer_event_t *self); /*  callback function called in
                                                   interrupt context */
         void *tev_data;                        /*  auxiliary data */
};

/*  call @tev callback after @tvdiff time has passed */
int cpu_timer_add_event(timer_event_t *tev, timeval_t *tvdiff);
int cpu_timer_remove_event(timer_event_t *tev);

/*  Processes core timer interrupts. */
void cpu_timer_irq_handler();

#endif /*  !_MIPS_CLOCK_H_ */
