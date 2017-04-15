#include <common.h>
#include <stdc.h>
#include <interrupt.h>

static TAILQ_HEAD(, intr_chain) intr_chain_list;

void intr_init() {
  TAILQ_INIT(&intr_chain_list);
}

void intr_chain_init(intr_chain_t *ic, unsigned irq, char *name) {
  ic->ic_irq = irq;
  ic->ic_count = 0;
  ic->ic_name = name;
  TAILQ_INIT(&ic->ic_handlers);
  TAILQ_INSERT_TAIL(&intr_chain_list, ic, ic_list);
}

void intr_chain_add_handler(intr_chain_t *ic, intr_handler_t *ih) {
  if (TAILQ_EMPTY(&ic->ic_handlers)) {
    TAILQ_INSERT_HEAD(&ic->ic_handlers, ih, ih_list);
  } else {
    /* Add new handler according to it's priority */
    intr_handler_t *it;

    TAILQ_FOREACH (it, &ic->ic_handlers, ih_list) {
      if (ih->ih_prio > it->ih_prio) {
        TAILQ_INSERT_BEFORE(it, ih, ih_list);
        goto done;
      }
    }
    TAILQ_INSERT_TAIL(&ic->ic_handlers, ih, ih_list);
  }

done:
  ih->ih_chain = ic;
  ic->ic_count++;
}

void intr_chain_remove_handler(intr_handler_t *ih) {
  intr_chain_t *ic = ih->ih_chain;
  TAILQ_REMOVE(&ic->ic_handlers, ih, ih_list);
  ih->ih_chain = NULL;
  ic->ic_count--;
}

/*
 * TODO when we have threads implement deferring work to thread.
 * With current implementation all filters have to either handle filter or
 * report that it is stray interrupt.
 */
void intr_chain_run_handlers(intr_chain_t *ic) {
  intr_handler_t *ih;
  intr_filter_t status = IF_STRAY;

  TAILQ_FOREACH (ih, &ic->ic_handlers, ih_list) {
    status |= ih->ih_filter ? ih->ih_filter(ih->ih_argument) : IF_HANDLED;
    if (status & IF_HANDLED) {
      ih->ih_handler(ih->ih_argument);
      return;
    }
  }

  log("Spurious %s interrupt!", ic->ic_name);
}
