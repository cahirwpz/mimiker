#include <intr_handler.h>

#include <common.h>
#include <queue.h>
#include <libkern.h>

intr_event_t *events[8];

void intr_event_add_handler(intr_event_t *ie, intr_handler_t *ih) {
  intr_handler_t *it;
  /* Add new handler according to it's priority */
  TAILQ_FOREACH(it, &ie->ie_handlers, ih_next) {
    if(ih->ih_prio > it->ih_prio)
      break;
  }
  if(it)
    TAILQ_INSERT_BEFORE(it, ih, ih_next);
  else
    /* List is empty */
    TAILQ_INSERT_TAIL(&ie->ie_handlers, ih, ih_next);
}

void run_event_handlers(unsigned irq) {
  intr_event_execute_handlers(events[irq]);
}

void intr_event_init(intr_event_t *ie, uint8_t irq, char *name) {
  ie->ie_rq = irq;
  ie->ie_name = name;
  TAILQ_INIT(&ie->ie_handlers);
  events[irq] = ie;
}

void intr_event_remove_handler(intr_handler_t *ih) {
  TAILQ_REMOVE(&ih->ih_event->ie_handlers, ih, ih_next);
}

/* TODO when we have threads implement deferring work to thread.
 * With current implementation all filters have to either handle filter or
 * report that it is stray interrupt.
 * */

void intr_event_execute_handlers(intr_event_t *ie) {
  intr_handler_t *it;
  int flag = 0;
  TAILQ_FOREACH(it, &ie->ie_handlers, ih_next) {
    flag |= it->ih_filter(it->ih_argument);
    /* Filter captured interrupt */
    if (flag & FILTER_HANDLED) return;
  }
#if 0
  /* clear flag */
  KASSERT(flag & FILTER_STRAY);
  int ifs_n = ie->ie_rq / 32;
  int intr_offset = (ie->ie_rq % 32);
  IFSCLR(ifs_n) = 1 << intr_offset;
#endif
}
