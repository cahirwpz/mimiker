#define KL_LOG KL_INTR
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <machine/interrupt.h>
#include <sys/malloc.h>
#include <sys/interrupt.h>
#include <sys/pcpu.h>
#include <sys/sleepq.h>
#include <sys/sched.h>

static KMALLOC_DEFINE(M_INTR, "interrupt events & handlers");

static mtx_t all_ievents_mtx = MTX_INITIALIZER(0);
static ie_list_t all_ievents_list = TAILQ_HEAD_INITIALIZER(all_ievents_list);

static void intr_thread(void *arg);

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

intr_event_t *intr_event_create(void *source, int irq, ie_action_t *disable,
                                ie_action_t *enable, const char *name) {
  intr_event_t *ie = kmalloc(M_INTR, sizeof(intr_event_t), M_WAITOK | M_ZERO);
  ie->ie_irq = irq;
  ie->ie_name = name;
  ie->ie_lock = SPIN_INITIALIZER(LK_RECURSIVE);
  ie->ie_enable = enable;
  ie->ie_disable = disable;
  ie->ie_source = source;
  ie->ie_ithread.it_thread = NULL;
  TAILQ_INIT(&ie->ie_handlers);

  WITH_MTX_LOCK (&all_ievents_mtx)
    TAILQ_INSERT_TAIL(&all_ievents_list, ie, ie_link);

  return ie;
}

/* Add new handler according to it's priority */
static void ie_insert_handler(intr_event_t *ie, intr_handler_t *ih) {
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

static void ie_add_handler(intr_event_t *ie, intr_handler_t *ih) {
  WITH_SPIN_LOCK (&ie->ie_lock) {
    ie_insert_handler(ie, ih);
    if (ie->ie_count == 1 && ie->ie_enable)
      ie->ie_enable(ie);
  }
}

intr_handler_t *intr_event_add_handler(intr_event_t *ie, ih_filter_t *filter,
                                       ih_service_t *service, void *arg,
                                       const char *name) {
  intr_handler_t *ih =
    kmalloc(M_INTR, sizeof(intr_handler_t), M_WAITOK | M_ZERO);
  ih->ih_filter = filter;
  ih->ih_service = service;
  ih->ih_argument = arg;
  ih->ih_name = name;
  ih->ih_prio = 0;
  ie_add_handler(ie, ih);
  return ih;
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
  /* XXX: Revisit possible data race when ithreads are implemented. */
  kfree(M_INTR, ih);
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
  assert(cpu_intr_disabled());

  intr_disable();
  PCPU_SET(no_switch, true);
  if (ir_filter != NULL)
    ir_filter(ctx, ir_dev, ir_arg);
  PCPU_SET(no_switch, false);
  intr_enable();

  on_exc_leave();
  if (user_mode_p(ctx))
    on_user_exc_leave();
}

/* interrupt handlers delegated to be called in the interrupt thread */
static ih_list_t delegated = TAILQ_HEAD_INITIALIZER(delegated);

void intr_create_ithread(intr_event_t *ie) {
  ie->ie_ithread.it_event = ie;
  ie->ie_ithread.it_thread =
    thread_create(ie->ie_name, intr_thread, &ie->ie_ithread, prio_ithread(0));
  ie->ie_ithread.it_delegated =
    (ih_list_t)TAILQ_HEAD_INITIALIZER(ie->ie_ithread.it_delegated);
  sched_add(ie->ie_ithread.it_thread);
}

void intr_event_run_handlers(intr_event_t *ie) {
  intr_handler_t *ih, *next;
  intr_filter_t status = IF_STRAY;

  assert(ie != NULL);

  TAILQ_FOREACH_SAFE (ih, &ie->ie_handlers, ih_link, next) {
    status = ih->ih_filter(ih->ih_argument);
    if (status == IF_FILTERED)
      return;
    if (status == IF_DELEGATE) {
      assert(ih->ih_service);

      if (ie->ie_disable)
        ie->ie_disable(ie);

      if (ie->ie_ithread.it_thread == NULL)
        intr_create_ithread(ie);

      TAILQ_REMOVE(&ie->ie_handlers, ih, ih_link);
      TAILQ_INSERT_TAIL(&ie->ie_ithread.it_delegated, ih, ih_ithread_link);
      sleepq_signal(&ie->ie_ithread.it_delegated);

      return;
    }
  }

  klog("Spurious %s interrupt!", ie->ie_name);
}

static void intr_thread(void *arg) {
  intr_thread_t *it = (intr_thread_t *)arg;
  ih_list_t *it_delegated = &it->it_delegated;

  while (true) {
    intr_handler_t *ih;

    WITH_INTR_DISABLED {
      while (TAILQ_EMPTY(it_delegated))
        sleepq_wait(it_delegated, NULL);
      ih = TAILQ_FIRST(it_delegated);
      TAILQ_REMOVE(it_delegated, ih, ih_ithread_link);
    }

    ih->ih_service(ih->ih_argument);

    intr_event_t *ie = ih->ih_event;

    WITH_SPIN_LOCK (&ie->ie_lock) {
      ie_insert_handler(ie, ih);
      if (ie->ie_enable)
        ie->ie_enable(ie);
    }
  }
}
