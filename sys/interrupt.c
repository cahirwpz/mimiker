#define KL_LOG KL_INTR
#include <klog.h>
#include <stdc.h>
#include <mutex.h>
#include <thread.h>
#include <mips/intr.h>
#include <interrupt.h>

static mtx_t all_ievents_mtx = MTX_INITIALIZER(0);
static intr_event_list_t all_ievents_list =
  TAILQ_HEAD_INITIALIZER(all_ievents_list);

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

void intr_event_init(intr_event_t *ie, unsigned irq, const char *name) {
  ie->ie_irq = irq;
  ie->ie_name = name;
  ie->ie_lock = SPIN_INITIALIZER(LK_RECURSE);
  TAILQ_INIT(&ie->ie_handlers);
}

void intr_event_register(intr_event_t *ie) {
  WITH_MTX_LOCK (&all_ievents_mtx)
    TAILQ_INSERT_TAIL(&all_ievents_list, ie, ie_list);
}

void intr_event_add_handler(intr_event_t *ie, intr_handler_t *ih) {
  SCOPED_SPIN_LOCK(&ie->ie_lock);

  /* Add new handler according to it's priority */
  intr_handler_t *it;
  TAILQ_FOREACH (it, &ie->ie_handlers, ih_list)
    if (ih->ih_prio > it->ih_prio)
      break;

  if (it)
    TAILQ_INSERT_BEFORE(it, ih, ih_list);
  else
    TAILQ_INSERT_TAIL(&ie->ie_handlers, ih, ih_list);

  ih->ih_event = ie;
  ie->ie_count++;
}

void intr_event_remove_handler(intr_handler_t *ih) {
  intr_event_t *ie = ih->ih_event;

  SCOPED_SPIN_LOCK(&ie->ie_lock);

  TAILQ_REMOVE(&ie->ie_handlers, ih, ih_list);
  ih->ih_event = NULL;
  ie->ie_count--;
}

/*
 * TODO when we have threads implement deferring work to thread.
 * With current implementation all filters have to either handle filter or
 * report that it is stray interrupt.
 */
void intr_event_run_handlers(intr_event_t *ie) {
  intr_handler_t *ih;
  intr_filter_t status = IF_STRAY;

  TAILQ_FOREACH (ih, &ie->ie_handlers, ih_list) {
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

  klog("Spurious %s interrupt!", ie->ie_name);
}
