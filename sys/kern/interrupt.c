#define KL_LOG KL_INTR
#include <sys/errno.h>
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/malloc.h>
#include <sys/interrupt.h>
#include <sys/cpu.h>
#include <sys/pcpu.h>
#include <sys/sleepq.h>
#include <sys/sched.h>
#include <sys/device.h>
#include <sys/fdt.h>
#include <dev/fdt_dev.h>

typedef struct pic pic_t;
typedef TAILQ_HEAD(, intr_event) ie_list_t;
typedef TAILQ_HEAD(, pic) pic_list_t;

struct pic {
  TAILQ_ENTRY(pic) link;
  device_t *dev;
  unsigned id;
};

static KMALLOC_DEFINE(M_INTR, "interrupt events & handlers");

static MTX_DEFINE(all_ievents_mtx, 0);
static ie_list_t all_ievents_list = TAILQ_HEAD_INITIALIZER(all_ievents_list);
static pic_list_t all_pic_list = TAILQ_HEAD_INITIALIZER(all_pic_list);

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

typedef struct intr_handler {
  TAILQ_ENTRY(intr_handler) ih_link;
  ih_filter_t *ih_filter;   /* interrupt filter routine (run in irq ctx) */
  ih_service_t *ih_service; /* interrupt service routine (run in thread ctx) */
  intr_event_t *ih_event;   /* event we are connected to */
  void *ih_argument;        /* argument to pass to filter/service routines */
  const char *ih_name;      /* name of the handler */
  ih_flags_t ih_flags;      /* refer to IH_* flags description above */
} intr_handler_t;

static void intr_thread(void *arg);

__no_profile bool intr_disabled(void) {
  thread_t *td = thread_self();
  return (td->td_idnest > 0) && cpu_intr_disabled();
}

__no_profile void intr_disable(void) {
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
  mtx_init(&ie->ie_lock, MTX_SPIN);
  ie->ie_enable = enable;
  ie->ie_disable = disable;
  ie->ie_source = source;
  ie->ie_ithread = NULL;
  TAILQ_INIT(&ie->ie_handlers);

  strlcpy(ie->ie_name, name, IENAMELEN);

  WITH_MTX_LOCK (&all_ievents_mtx)
    TAILQ_INSERT_TAIL(&all_ievents_list, ie, ie_link);

  return ie;
}

static void ie_enable(intr_event_t *ie) {
  if (ie->ie_enable)
    ie->ie_enable(ie);
}

static void ie_disable(intr_event_t *ie) {
  if (ie->ie_disable)
    ie->ie_disable(ie);
}

static void intr_event_insert_handler(intr_event_t *ie, intr_handler_t *ih) {
  SCOPED_MTX_LOCK(&ie->ie_lock);

  /* Enable interrupt if this is the first handler. */
  if (TAILQ_EMPTY(&ie->ie_handlers))
    ie_enable(ie);

  TAILQ_INSERT_TAIL(&ie->ie_handlers, ih, ih_link);
}

static void intr_thread_maybe_attach(intr_event_t *ie, intr_handler_t *ih) {
  /* Ensure we can create interrupt thread only once! */
  WITH_MTX_LOCK (&ie->ie_lock) {
    if (ie->ie_ithread != NULL || ih->ih_service == NULL)
      return;
    /* We can't execute `thread_create` under spin lock, thus mark ie_thread
     * so that no other thread will attempt to execute code below. */
    ie->ie_ithread = (thread_t *)1L;
  }

  ie->ie_ithread = thread_create(ie->ie_name, intr_thread, ie, prio_ithread(0));
  sched_add(ie->ie_ithread);
}

intr_handler_t *intr_event_add_handler(intr_event_t *ie, ih_filter_t *filter,
                                       ih_service_t *service, void *arg,
                                       const char *name) {
  intr_handler_t *ih =
    kmalloc(M_INTR, sizeof(intr_handler_t), M_WAITOK | M_ZERO);
  ih->ih_filter = filter;
  ih->ih_service = service;
  ih->ih_event = ie;
  ih->ih_argument = arg;
  ih->ih_name = name;
  ih->ih_flags = 0;
  intr_event_insert_handler(ie, ih);
  intr_thread_maybe_attach(ie, ih);
  return ih;
}

void intr_event_remove_handler(intr_handler_t *ih) {
  intr_event_t *ie = ih->ih_event;
  WITH_MTX_LOCK (&ie->ie_lock) {
    if (ih->ih_flags & IH_DELEGATE) {
      ih->ih_flags |= IH_REMOVE;
      return;
    }

    TAILQ_REMOVE(&ie->ie_handlers, ih, ih_link);

    if (TAILQ_EMPTY(&ie->ie_handlers))
      ie_disable(ie);
  }
  kfree(M_INTR, ih);
}

static intr_root_filter_t ir_filter;
static device_t *ir_dev;

void intr_root_claim(intr_root_filter_t filter, device_t *dev) {
  assert(filter != NULL);

  ir_filter = filter;
  ir_dev = dev;
}

__no_profile void intr_root_handler(ctx_t *ctx) {
  thread_t *td = thread_self();

  assert(cpu_intr_disabled());
  assert(td->td_idnest == 0);

  /* Increment interrupt disable nesting counter in order to prevent filter
   * routines to accidentaly enable interrupts. */
  td->td_idnest++;

  /* Explicitely disallow switching out to another thread. */
  PCPU_SET(no_switch, true);
  if (ir_filter != NULL)
    ir_filter(ctx, ir_dev);
  PCPU_SET(no_switch, false);

  /* If filter routine requested a context switch it's now time to handle it. */
  sched_maybe_switch();

  /* To avoid `intr_root_handler` nesting while in kernel mode,
   * we have to complete this routine without interrupts enabled.
   * If we came from user mode, then we need to configure the context
   * to perform signal processing, which can be interrupted and that's ok. */
  if (user_mode_p(ctx)) {
    intr_enable();
    sig_userret((mcontext_t *)ctx, NULL);
    return;
  }

  td->td_idnest--;
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
    ie_disable(ie);
    sleepq_signal(ie);
  }

  if (ie_status == IF_STRAY)
    klog("Spurious %s interrupt!", ie->ie_name);
}

void intr_pic_register(device_t *pic, unsigned id) {
  assert(pic);

  pic_t *p = kmalloc(M_DEV, sizeof(pic_t), M_WAITOK | M_ZERO);
  p->dev = pic;
  p->id = id;

  TAILQ_INSERT_TAIL(&all_pic_list, p, link);
}

static device_t *intr_pic_find(unsigned id) {
  pic_t *pic;
  TAILQ_FOREACH (pic, &all_pic_list, link) {
    if (pic->id == id)
      return pic->dev;
  }
  return NULL;
}

static inline pic_methods_t *pic_methods(device_t *dev) {
  return (pic_methods_t *)dev->driver->interfaces[DIF_PIC];
}

int pic_setup_intr(device_t *dev, dev_intr_t *intr, ih_filter_t *filter,
                   ih_service_t *service, void *arg, const char *name) {
  assert(!intr->handler);
  device_t *pic = intr->pic;
  if (pic) {
    if (intr->irq == -1)
      return EINVAL;
    goto setup;
  }

  if (!(pic = intr_pic_find(intr->pic_id)))
    return EAGAIN;
  intr->pic = pic;

  int err = pic_map_intr(dev, intr);
  if (err)
    return err;

setup:
  pic_methods(pic)->setup_intr(pic, dev, intr, filter, service, arg, name);
  return 0;
}

void pic_teardown_intr(device_t *dev, dev_intr_t *intr) {
  assert(intr->pic);
  assert(intr->handler);
  assert(intr->irq >= 0);
  device_t *pic = intr->pic;
  pic_methods(pic)->teardown_intr(pic, dev, intr);
  intr->handler = NULL;
}

int pic_map_intr(device_t *dev, dev_intr_t *intr) {
  device_t *pic = intr->pic;

  if (!pic_methods(pic)->map_intr)
    return 0;

  pcell_t cells[FDT_MAX_ICELLS];
  size_t ncells;
  int err = FDT_dev_get_intr_cells(dev, intr, cells, &ncells);
  if (err)
    return err;

  intr->irq = pic_methods(pic)->map_intr(pic, dev, cells, ncells);
  return 0;
}

static void intr_thread(void *arg) {
  intr_event_t *ie = (intr_event_t *)arg;

  /* When we enter intr_thread `ie`-specific interrupt may not have been
   * disabled, but we execute the loop with that assumption. Fix that. */
  ie_disable(ie);

  while (true) {
    intr_handler_t *ih, *ih_next;

    /* The interrupt associated with `ie` was disabled by
     * `intr_event_run_handlers`, so it's safe to use `ie` now. */
    TAILQ_FOREACH_SAFE (ih, &ie->ie_handlers, ih_link, ih_next) {
      if (ih->ih_flags & IH_DELEGATE) {
        ih->ih_service(ih->ih_argument);
        ih->ih_flags &= ~IH_DELEGATE;

        if (ih->ih_flags & IH_REMOVE) {
          TAILQ_REMOVE(&ie->ie_handlers, ih, ih_link);
          kfree(M_INTR, ih);
        }
      }
    }

    /* If there are still handlers assigned to the interrupt event, enable
     * interrupts and wait for a wakeup. We do it with interrupts disabled
     * to prevent the wakeup from being lost. */
    WITH_INTR_DISABLED {
      if (!TAILQ_EMPTY(&ie->ie_handlers))
        ie_enable(ie);

      sleepq_wait(ie, NULL);
    }
  }
}
