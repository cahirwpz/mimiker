#define KL_LOG KL_INTR
#include <klog.h>
#include <stdc.h>
#include <mutex.h>
#include <thread.h>
#include <mips/intr.h>
#include <interrupt.h>
#include <sched.h>
#include <sleepq.h>

static mtx_t all_ichains_mtx = MTX_INITIALIZER(0);
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

void intr_chain_init(intr_chain_t *ic, unsigned irq, const char *name) {
  ic->ic_irq = irq;
  ic->ic_name = name;
  ic->ic_lock = SPIN_INITIALIZER(LK_RECURSE);
  TAILQ_INIT(&ic->ic_handlers);
}

void intr_chain_register(intr_chain_t *ic) {
  WITH_MTX_LOCK (&all_ichains_mtx)
    TAILQ_INSERT_TAIL(&all_ichains_list, ic, ic_list);
}

// TODO come up with better name
static void intr_chain_tailq_insert(intr_chain_t *ic, intr_handler_t *ih) {
  intr_handler_t *it;
  TAILQ_FOREACH (it, &ic->ic_handlers, ih_list)
    if (ih->ih_prio > it->ih_prio)
      break;

  if (it)
    TAILQ_INSERT_BEFORE(it, ih, ih_list);
  else
    TAILQ_INSERT_TAIL(&ic->ic_handlers, ih, ih_list);
}

void intr_chain_add_handler(intr_chain_t *ic, intr_handler_t *ih) {
  SCOPED_SPIN_LOCK(&ic->ic_lock);

  /* Add new handler according to it's priority */
  intr_chain_tailq_insert(ic, ih);

  ih->ih_chain = ic;
  ic->ic_count++;
}

void intr_chain_remove_handler(intr_handler_t *ih) {
  intr_chain_t *ic = ih->ih_chain;

  SCOPED_SPIN_LOCK(&ic->ic_lock);

  TAILQ_REMOVE(&ic->ic_handlers, ih, ih_list);
  ih->ih_chain = NULL;
  ic->ic_count--;
}

typedef TAILQ_HEAD(, intr_handler) ih_list_t;
static ih_list_t delegated;

void intr_thread(void *arg) {
  while (true) {
    intr_handler_t *elem;

    WITH_INTR_DISABLED {
      while (TAILQ_EMPTY(&delegated)) {
        sleepq_wait(&delegated, NULL);
      }
      elem = TAILQ_FIRST(&delegated);
      TAILQ_REMOVE(&delegated, elem, ih_list);
    }

    elem->ih_handler(elem->ih_argument);

    WITH_INTR_DISABLED {
      intr_chain_tailq_insert(elem->ih_chain, elem);
      if (elem->ih_eoi != NULL) {
        elem->ih_eoi(elem->ih_argument);
      }
    }
  }
}

void intr_chain_run_handlers(intr_chain_t *ic) {
  intr_handler_t *ih, *next;
  intr_filter_t status = IF_STRAY;

  TAILQ_FOREACH_SAFE(ih, &ic->ic_handlers, ih_list, next) {
    assert(ih->ih_filter != NULL);
    status = ih->ih_filter(ih->ih_argument);
    if (status == IF_FILTERED) {
      if (ih->ih_eoi != NULL) {
        ih->ih_eoi(ih->ih_argument);
      }
      return;
    } else if (status == IF_DELEGATE) {
      assert(ih->ih_handler != NULL);

      TAILQ_REMOVE(&ic->ic_handlers, ih, ih_list);
      TAILQ_INSERT_TAIL(&delegated, ih, ih_list);
      sleepq_signal(&delegated);
      return;
    }
  }

  klog("Spurious %s interrupt!", ic->ic_name);
}
