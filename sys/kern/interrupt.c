#define KL_LOG KL_INTR
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <machine/interrupt.h>
#include <sys/interrupt.h>
#include <sys/sleepq.h>
#include <sys/sched.h>

static mtx_t all_ievents_mtx = MTX_INITIALIZER(0);
static ie_list_t all_ievents_list = TAILQ_HEAD_INITIALIZER(all_ievents_list);

bool intr_disabled(void) {
  thread_t *td = thread_self();
  return (td->td_idnest > 0) && cpu_intr_disabled();
}

void intr_disable(void) {
  cpu_intr_disable();
  thread_self()->td_idnest++;
}

void intr_enable(void) {
  assert(intr_disabled());
  thread_t *td = thread_self();
  td->td_idnest--;
  if (td->td_idnest == 0)
    cpu_intr_enable();
}

void intr_event_init(intr_event_t *ie, unsigned irq, const char *name,
                     ie_action_t *disable, ie_action_t *enable, void *source) {
  ie->ie_irq = irq;
  ie->ie_name = name;
  ie->ie_lock = SPIN_INITIALIZER(LK_RECURSIVE);
  ie->ie_enable = enable;
  ie->ie_disable = disable;
  ie->ie_source = source;
  TAILQ_INIT(&ie->ie_handlers);
}

void intr_event_register(intr_event_t *ie) {
  WITH_MTX_LOCK (&all_ievents_mtx)
    TAILQ_INSERT_TAIL(&all_ievents_list, ie, ie_link);
}

/* Add new handler according to it's priority */
static void insert_handler(intr_event_t *ie, intr_handler_t *ih) {
  intr_handler_t *it;
  TAILQ_FOREACH (it, &ie->ie_handlers, ih_link)
    if (ih->ih_prio > it->ih_prio)
      break;

  if (it)
    TAILQ_INSERT_BEFORE(it, ih, ih_link);
  else
    TAILQ_INSERT_TAIL(&ie->ie_handlers, ih, ih_link);

  ih->ih_event = ie;
  ie->ie_count++;
}

void intr_event_add_handler(intr_event_t *ie, intr_handler_t *ih) {
  WITH_SPIN_LOCK (&ie->ie_lock) {
    insert_handler(ie, ih);
    if (ie->ie_count == 1 && ie->ie_enable)
      ie->ie_enable(ie);
  }
}

void intr_event_remove_handler(intr_handler_t *ih) {
  intr_event_t *ie = ih->ih_event;
  WITH_SPIN_LOCK (&ie->ie_lock) {
    if (ie->ie_count == 1 && ie->ie_disable)
      ie->ie_disable(ie);

    TAILQ_REMOVE(&ie->ie_handlers, ih, ih_link);
    ih->ih_event = NULL;
    ie->ie_count--;
  }
}

/* interrupt handlers delegated to be called in the interrupt thread */
static ih_list_t delegated = TAILQ_HEAD_INITIALIZER(delegated);

void intr_event_run_handlers(intr_event_t *ie) {
  intr_handler_t *ih, *next;
  intr_filter_t status = IF_STRAY;

  TAILQ_FOREACH_SAFE (ih, &ie->ie_handlers, ih_link, next) {
    status = ih->ih_filter(ih->ih_argument);
    if (status == IF_FILTERED)
      return;
    if (status == IF_DELEGATE) {
      assert(ih->ih_service);

      if (ie->ie_disable)
        ie->ie_disable(ie);

      TAILQ_REMOVE(&ie->ie_handlers, ih, ih_link);
      TAILQ_INSERT_TAIL(&delegated, ih, ih_link);
      sleepq_signal(&delegated);
      return;
    }
  }

  klog("Spurious %s interrupt!", ie->ie_name);
}

static intr_root_filter_t ir_filter;
static device_t *ir_dev;
static void *ir_arg;

void intr_root_claim(intr_root_filter_t filter, device_t *dev, void *arg) {
  assert(filter != NULL);

  ir_filter = filter;
  ir_dev = dev;
  ir_arg = arg;
}

void intr_root_handler(ctx_t *ctx) {
  intr_disable();
  if (ir_filter != NULL)
    ir_filter(ir_dev, ir_arg);
  intr_enable();
  on_exc_leave();
}

static void intr_thread(void *arg) {
  while (true) {
    intr_handler_t *ih;

    WITH_INTR_DISABLED {
      while (TAILQ_EMPTY(&delegated))
        sleepq_wait(&delegated, NULL);
      ih = TAILQ_FIRST(&delegated);
      TAILQ_REMOVE(&delegated, ih, ih_link);
    }

    ih->ih_service(ih->ih_argument);

    intr_event_t *ie = ih->ih_event;

    WITH_SPIN_LOCK (&ie->ie_lock) {
      insert_handler(ie, ih);
      if (ie->ie_enable)
        ie->ie_enable(ie);
    }
  }
}

void init_ithreads(void) {
  thread_t *itd =
    thread_create("interrupt", intr_thread, NULL, prio_ithread(0));
  sched_add(itd);
}
