#define KL_LOG KL_INTR
#include <klog.h>
#include <stdc.h>
#include <mutex.h>
#include <thread.h>
#include <mips/intr.h>
#include <interrupt.h>
#include <sleepq.h>

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

void intr_event_init(intr_event_t *ie, unsigned irq, const char *name,
                     driver_mask_t *mask_irq, driver_mask_t *unmask_irq,
                     void *source) {
  ie->ie_irq = irq;
  ie->ie_name = name;
  ie->ie_lock = SPIN_INITIALIZER(LK_RECURSE);
  ie->ie_mask_irq = mask_irq;
  ie->ie_unmask_irq = unmask_irq;
  ie->ie_source = source;
  TAILQ_INIT(&ie->ie_handlers);
}

void intr_event_register(intr_event_t *ie) {
  WITH_MTX_LOCK (&all_ievents_mtx)
    TAILQ_INSERT_TAIL(&all_ievents_list, ie, ie_list);
}

static void tailq_handler_insert(intr_event_t *ie, intr_handler_t *ih) {
  intr_handler_t *it;
  TAILQ_FOREACH (it, &ie->ie_handlers, ih_list)
    if (ih->ih_prio > it->ih_prio)
      break;

  if (it)
    TAILQ_INSERT_BEFORE(it, ih, ih_list);
  else
    TAILQ_INSERT_TAIL(&ie->ie_handlers, ih, ih_list);
}

void intr_event_add_handler(intr_event_t *ie, intr_handler_t *ih) {
  SCOPED_SPIN_LOCK(&ie->ie_lock);

  /* Add new handler according to it's priority */
  tailq_handler_insert(ie, ih);

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

static void run_mask_irq(intr_handler_t *ih) {
  intr_event_t *ie = ih->ih_event;
  if (ie->ie_mask_irq != NULL) {
    ie->ie_mask_irq(ie);
  }
}

static void run_unmask_irq(intr_handler_t *ih) {
  intr_event_t *ie = ih->ih_event;
  if (ie->ie_unmask_irq != NULL) {
    ie->ie_unmask_irq(ie);
  }
}

typedef TAILQ_HEAD(, intr_handler) ih_list_t;
/* interrupt handlers delegated to be called in the interrupt thread */
static ih_list_t delegated = TAILQ_HEAD_INITIALIZER(delegated);

void intr_thread(void *arg) {
  while (true) {
    intr_handler_t *ih;

    WITH_INTR_DISABLED {
      while (TAILQ_EMPTY(&delegated)) {
        sleepq_wait(&delegated, NULL);
      }
      ih = TAILQ_FIRST(&delegated);
      TAILQ_REMOVE(&delegated, ih, ih_list);
    }

    ih->ih_handler(ih->ih_argument);

    WITH_SPIN_LOCK (&ih->ih_event->ie_lock) {
      tailq_handler_insert(ih->ih_event, ih);
      run_unmask_irq(ih);
    }
  }
}

void intr_event_run_handlers(intr_event_t *ie) {
  intr_handler_t *ih, *next;
  intr_filter_t status = IF_STRAY;

  TAILQ_FOREACH_SAFE(ih, &ie->ie_handlers, ih_list, next) {
    status = ih->ih_filter(ih->ih_argument);
    if (status == IF_FILTERED) {
      return;
    } else if (status == IF_DELEGATE) {
      assert(ih->ih_handler != NULL);

      run_mask_irq(ih);

      TAILQ_REMOVE(&ie->ie_handlers, ih, ih_list);
      TAILQ_INSERT_TAIL(&delegated, ih, ih_list);
      sleepq_signal(&delegated);
      return;
    }
  }

  klog("Spurious %s interrupt!", ie->ie_name);
}
