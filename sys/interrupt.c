#define KL_LOG KL_INTR
#include <klog.h>
#include <stdc.h>
#include <mutex.h>
#include <thread.h>
#include <mips/intr.h>
#include <interrupt.h>

static mtx_t all_ichains_mtx = MTX_INITIALIZER(MTX_DEF);
static intr_chain_list_t all_ichains_list =
  TAILQ_HEAD_INITIALIZER(all_ichains_list);

bool intr_disabled(void) {
  thread_t *td = thread_self();
  return (td->td_idnest > 0) && mips_intr_disabled();
}

void intr_disable(void) {
  mips_intr_disable();
  thread_self()->td_idnest++;
}

void intr_enable(void) {
  assert(intr_disabled());
  thread_t *td = thread_self();
  td->td_idnest--;
  if (td->td_idnest == 0)
    mips_intr_enable();
}

void intr_chain_register(intr_chain_t *ic) {
  WITH_MTX_LOCK (&all_ichains_mtx)
    TAILQ_INSERT_TAIL(&all_ichains_list, ic, ic_list);
}

void intr_chain_add_handler(intr_chain_t *ic, intr_handler_t *ih) {
  SCOPED_SPINLOCK(&ic->ic_lock);

  /* Add new handler according to it's priority */
  intr_handler_t *it;
  TAILQ_FOREACH (it, &ic->ic_handlers, ih_list)
    if (ih->ih_prio > it->ih_prio)
      break;

  if (it)
    TAILQ_INSERT_BEFORE(it, ih, ih_list);
  else
    TAILQ_INSERT_TAIL(&ic->ic_handlers, ih, ih_list);

  ih->ih_chain = ic;
  ic->ic_count++;
}

void intr_chain_remove_handler(intr_handler_t *ih) {
  intr_chain_t *ic = ih->ih_chain;

  SCOPED_SPINLOCK(&ic->ic_lock);

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
    assert(ih->ih_filter != NULL);
    status = ih->ih_filter(ih->ih_argument);
    if (status == IF_FILTERED)
      return;
    if (status == IF_DELEGATE) {
      assert(ih->ih_handler != NULL);
      /* TODO: delegate the handler to be run in interrupt thread context */
      ih->ih_handler(ih->ih_argument);
      return;
    }
  }

  klog("Spurious %s interrupt!", ic->ic_name);
}
