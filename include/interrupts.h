#ifndef __SYS_INTERRUPTS_H__
#define __SYS_INTERRUPTS_H__

#include <common.h>
#include <queue.h>

#define FILTER_STRAY   0x01 /* this device did not trigger the interrupt */
#define FILTER_HANDLED 0x02 /* the interrupt has been fully handled and can be EOId */

typedef int driver_filter_t(void *);
typedef void driver_intr_t(void *);
typedef int prio_t;

/* Forward declaration to allow nested type */
typedef struct intr_event intr_event_t;

typedef struct intr_handler {
  TAILQ_ENTRY(intr_handler) ih_next;
  driver_filter_t *ih_filter;
  intr_event_t *ih_event;
  void *ih_argument;
  char *ih_namel;
  prio_t ih_prio;
} intr_handler_t;

typedef struct intr_event {
  TAILQ_ENTRY(intr_event) ie_list;
  TAILQ_HEAD(,intr_handler) ie_handlers;
  char *ie_name;
  uint8_t ie_rq;
} intr_event_t;

/* Initializes and enables interrupts. */
void intr_init();

void intr_event_add_handler(intr_event_t *ie, intr_handler_t *ih);
void intr_event_init(intr_event_t *ie, uint8_t irq, char *name);
void intr_event_remove_handler(intr_handler_t *ih);
void intr_event_execute_handlers(intr_event_t *ie);
void run_event_handlers(unsigned irq);

#define intr_disable() __extension__ ({ asm("di"); })
#define intr_enable() __extension__ ({ asm("ei"); })

const char *const exceptions[32];

#endif /* __SYS_INTERRUPTS_H__ */
