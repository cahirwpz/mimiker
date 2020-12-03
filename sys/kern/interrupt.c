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
  ie->ie_ithread = NULL;
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
  ih->ih_flags = 0;
  ie_add_handler(ie, ih);
  return ih;
}

/* XXX: This should always be called in a spin lock */
static void intr_event_delete_handler(intr_handler_t *ih) {
  intr_event_t *ie = ih->ih_event;
  if (ie->ie_count == 1 && ie->ie_disable)
    ie->ie_disable(ie);

  TAILQ_REMOVE(&ie->ie_handlers, ih, ih_link);
  ih->ih_event = NULL;
  ie->ie_count--;
}

void intr_event_remove_handler(intr_handler_t *ih) {
  intr_event_t *ie = ih->ih_event;
  WITH_SPIN_LOCK (&ie->ie_lock) {
    if (ih->ih_flags & IH_DELEGATE) {
      ih->ih_flags |= IH_REMOVE;
      return;
    }

    intr_event_delete_handler(ih);
  }
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

static void intr_thread_create(intr_event_t *ie) {
  intr_thread_t *it = kmalloc(M_INTR, sizeof(intr_thread_t), M_ZERO);

  assert(it);

  ie->ie_ithread = it;

  it->it_thread = thread_create(ie->ie_name, intr_thread, it, prio_ithread(0));

  assert(it->it_thread != NULL);

  sched_add(it->it_thread);
}

void intr_event_run_handlers(intr_event_t *ie) {
  intr_handler_t *ih, *next;
  intr_filter_t status = IF_STRAY;

  assert(ie != NULL);

  /* Do we wake up an ithread */
  bool it_wakeup = false;

  TAILQ_FOREACH_SAFE (ih, &ie->ie_handlers, ih_link, next) {
    status = ih->ih_filter(ih->ih_argument);
    if (status == IF_FILTERED)
      continue;
    if (status == IF_DELEGATE) {
      assert(ih->ih_service);

      if (ie->ie_disable)
        ie->ie_disable(ie);

      ih->ih_flags = IH_DELEGATE;

      it_wakeup = true;
    }
  }

  if (it_wakeup) {
    if (ie->ie_ithread == NULL)
      intr_thread_create(ie);

    sleepq_signal(ie->ie_ithread);
  }

  klog("Spurious %s interrupt!", ie->ie_name);
}

static void intr_thread(void *arg) {
  intr_thread_t *it = (intr_thread_t *)arg;

  while (true) {
    intr_event_t *ie = it->it_event;

    intr_handler_t *ih = NULL;

    /* We look at the list of handlers of our event and search for a handler
     * marked as IH_DELEGATE. If we don't find any handlers to be serviced, we
     * go to sleep.
     */
    do {
      WITH_INTR_DISABLED {
        TAILQ_FOREACH (ih, &ie->ie_handlers, ih_link) {
          if (ih->ih_flags & IH_DELEGATE)
            break;
        }
        if (ih == NULL)
          sleepq_wait(it, NULL);
      }
    } while (it == NULL);

    ih->ih_service(ih->ih_argument);

    WITH_SPIN_LOCK (&ie->ie_lock) {

      /* If the handler is flagged as IH_REMOVE, we remove it similarly to how
       * we do it in intr_event_remove_handler. We are under a spin lock so we
       * don't call kfree just yet.
       */
      if (ih->ih_flags & IH_REMOVE) {
        intr_event_delete_handler(ih);
      } else {
        /* If the handler is not flagged as IH_REMOVE, we
         * remove the IH_DELEGATE flag and enable interrupts from the event
         * again.
         */
        ih->ih_flags &= ~IH_DELEGATE;
        if (ie->ie_enable)
          ie->ie_enable(ie);
      }
    }
    /* If the handler is flagged as IH_REMOVE, we kfree its memory here. We
     * didn't want to do it in the code above, because we were under a spinlock
     */
    if (ih->ih_flags & IH_REMOVE) {
      kfree(M_INTR, ih);
    }
  }
}
