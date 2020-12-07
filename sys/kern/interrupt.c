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

typedef TAILQ_HEAD(, intr_event) ie_list_t;

static mtx_t all_ievents_mtx = MTX_INITIALIZER(0);
static ie_list_t all_ievents_list = TAILQ_HEAD_INITIALIZER(all_ievents_list);

/*
 * IH_REMOVE: set when the handler cannot be removed, because it has been
 * delegated to an interrupt thread. The thread will free the handler after
 * ih_service has been executed.
 *
 * IH_DELEGATE: set when ih_service function was delegated to an interrupt
 * thread for execution.
 */
typedef enum {
  IH_REMOVE = 1,
  IH_DELEGATE = 2,
} ih_flags_t;

typedef struct intr_thread {
  intr_event_t *it_event; /* Associated event */
  thread_t *it_thread;    /* Kernel thread. */
} intr_thread_t;

typedef struct intr_handler {
  TAILQ_ENTRY(intr_handler) ih_link;
  ih_filter_t *ih_filter;   /* interrupt filter routine (run in irq ctx) */
  ih_service_t *ih_service; /* interrupt service routine (run in thread ctx) */
  intr_event_t *ih_event;   /* event we are connected to */
  void *ih_argument;        /* argument to pass to filter/service routines */
  const char *ih_name;      /* name of the handler */
  /* XXX: do we really need ih_prio? it has no real use cases so far... */
  prio_t ih_prio;      /* handler's priority (sort key for ie_handlers) */
  ih_flags_t ih_flags; /* refer to IH_* flags description above */
} intr_handler_t;

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

static void intr_event_insert_handler(intr_event_t *ie, intr_handler_t *ih) {
  intr_handler_t *it;

  WITH_SPIN_LOCK (&ie->ie_lock) {
    /* Add new handler according to it's priority */
    TAILQ_FOREACH (it, &ie->ie_handlers, ih_link)
      if (ih->ih_prio > it->ih_prio)
        break;

    if (it)
      TAILQ_INSERT_BEFORE(it, ih, ih_link);
    else
      TAILQ_INSERT_TAIL(&ie->ie_handlers, ih, ih_link);

    ih->ih_event = ie;
    ie->ie_count++;

    if (ie->ie_count == 1 && ie->ie_enable)
      ie->ie_enable(ie);
  }
}

static void intr_thread_create(intr_event_t *ie) {
  intr_thread_t *it = kmalloc(M_INTR, sizeof(intr_thread_t), M_ZERO);

  ie->ie_ithread = it;
  it->it_event = ie;
  it->it_thread = thread_create(ie->ie_name, intr_thread, it, prio_ithread(0));

  sched_add(it->it_thread);
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
  intr_event_insert_handler(ie, ih);
  if (service != NULL && ie->ie_ithread == NULL)
    intr_thread_create(ie);
  return ih;
}

void intr_event_remove_handler(intr_handler_t *ih) {
  intr_event_t *ie = ih->ih_event;
  WITH_SPIN_LOCK (&ie->ie_lock) {
    if (ih->ih_flags & IH_DELEGATE) {
      ih->ih_flags |= IH_REMOVE;
      return;
    }

    if (ie->ie_count == 1 && ie->ie_disable)
      ie->ie_disable(ie);

    TAILQ_REMOVE(&ie->ie_handlers, ih, ih_link);
    ie->ie_count--;
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

void intr_event_run_handlers(intr_event_t *ie) {
  intr_handler_t *ih, *next;

  assert(intr_disabled());
  assert(ie != NULL);

  /* Do we wake up an ithread */
  intr_filter_t ie_status = IF_STRAY;

  TAILQ_FOREACH_SAFE (ih, &ie->ie_handlers, ih_link, next) {
    intr_filter_t status = ih->ih_filter(ih->ih_argument);

    ie_status |= status;

    if (status == IF_DELEGATE) {
      assert(ih->ih_service);
      ih->ih_flags = IH_DELEGATE;
    }
  }

  if (ie_status & IF_DELEGATE) {
    if (ie->ie_disable)
      ie->ie_disable(ie);
    sleepq_signal(ie->ie_ithread);
  }

  if (ie_status == IF_STRAY)
    klog("Spurious %s interrupt!", ie->ie_name);
}

static void intr_thread(void *arg) {
  intr_thread_t *it = (intr_thread_t *)arg;

  while (true) {
    intr_event_t *ie = it->it_event;
    intr_handler_t *ih = NULL;

    /* The interrupt associated with `ie` was disabled by
     * `intr_event_run_handlers`, so it's safe to use `ie` now. */
    TAILQ_FOREACH (ih, &ie->ie_handlers, ih_link) {
      if (ih->ih_flags & IH_DELEGATE) {
        ih->ih_service(ih->ih_argument);
        ih->ih_flags &= ~IH_DELEGATE;

        if (ih->ih_flags & IH_REMOVE) {
          TAILQ_REMOVE(&ie->ie_handlers, ih, ih_link);
          ie->ie_count--;
          kfree(M_INTR, ih);
        }
      }
    }

    /* If there are still handlers assign to the interrupt event, enable
     * interrupts and wait for a wakeup. We do it with interrupts disabled
     * to prevent a wakeup from being lost. */
    WITH_INTR_DISABLED {
      if (ie->ie_count > 0 && ie->ie_enable)
        ie->ie_enable(ie);

      sleepq_wait(it, NULL);
    }
  }
}
