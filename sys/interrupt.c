#include <interrupt.h>

static TAILQ_HEAD(, intr_chain) intr_chain_list;

void intr_init() {
  TAILQ_INIT(&intr_chain_list);
}

void intr_chain_init(intr_chain_t *ic, unsigned irq, char *name) {
  ic->ic_irq = irq;
  ic->ic_name = name;
  TAILQ_INIT(&ic->ic_handlers);
  TAILQ_INSERT_TAIL(&intr_chain_list, ic, ic_list);
}

void intr_chain_add_handler(intr_chain_t *ic, intr_handler_t *ih) {
  intr_handler_t *it;
  /* Add new handler according to it's priority */
  TAILQ_FOREACH (it, &ic->ic_handlers, ih_list) {
    if (ih->ih_prio > it->ih_prio)
      break;
  }
  if (it) {
    TAILQ_INSERT_BEFORE(it, ih, ih_list);
  } else {
    /* List is empty */
    TAILQ_INSERT_HEAD(&ic->ic_handlers, ih, ih_list);
  }
}

void intr_chain_remove_handler(intr_handler_t *ih) {
  TAILQ_REMOVE(&ih->ih_chain->ic_handlers, ih, ih_list);
}

/*
 * TODO when we have threads implement deferring work to thread.
 * With current implementation all filters have to either handle filter or
 * report that it is stray interrupt.
 */
void intr_chain_execute_handlers(intr_chain_t *ic) {
  intr_handler_t *it;
  int flag = 0;
  TAILQ_FOREACH (it, &ic->ic_handlers, ih_list) {
    flag |= it->ih_filter(it->ih_argument);
    /* Filter captured interrupt */
    if (flag & IF_HANDLED)
      return;
  }
}
