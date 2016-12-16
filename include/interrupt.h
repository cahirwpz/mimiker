#ifndef _SYS_INTERRUPT_H_
#define _SYS_INTERRUPT_H_

#include <common.h>
#include <queue.h>

typedef enum {
  IF_STRAY = 1,           /* this device did not trigger the interrupt */
  IF_HANDLED = 2,         /* the interrupt has been handled and can be EOId */
  IF_SCHEDULE_THREAD = 4, /* the handler should be run in private thread */
} intr_filter_t;

/*
 * The filter routine is run in primary interrupt context and may not
 * block or use regular mutexes.  The filter may either completely
 * handle the interrupt or it may perform some of the work and
 * defer more expensive work to the regular interrupt handler.
 */
typedef intr_filter_t driver_filter_t(void *);
typedef void driver_intr_t(void *);

typedef struct intr_chain intr_chain_t;
typedef int prio_t;

typedef struct intr_handler {
  TAILQ_ENTRY(intr_handler) ih_list;
  driver_filter_t *ih_filter; /* driver interrupt filter function */
  driver_intr_t *ih_handler;  /* driver interrupt handler function */
  intr_chain_t *ih_chain;     /* chain we are connected to */
  void *ih_argument;          /* argument to pass to the handler */
  char *ih_name;              /* name of the handler */
  prio_t ih_prio;             /* priority of the handler */
} intr_handler_t;

typedef TAILQ_HEAD(, intr_handler) intr_handler_list_t;

typedef struct intr_chain {
  TAILQ_ENTRY(intr_chain) ic_list;
  intr_handler_list_t ic_handlers; /* interrupt handlers */
  char *ic_name;   /* individual chain name */
  unsigned ic_irq; /* physical interrupt request line number */
} intr_chain_t;

/* Initializes and enables interrupts. */
void intr_init();

void intr_chain_init(intr_chain_t *ic, unsigned irq, char *name);
void intr_chain_add_handler(intr_chain_t *ic, intr_handler_t *ih);
void intr_chain_remove_handler(intr_handler_t *ih);
void intr_chain_run_handlers(intr_chain_t *ic);

#endif /* _SYS_INTERRUPT_H_ */
