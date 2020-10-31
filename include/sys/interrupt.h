#ifndef _SYS_INTERRUPT_H_
#define _SYS_INTERRUPT_H_

#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/spinlock.h>
#include <sys/priority.h>

/*! \brief Called during kernel initialization. */
void init_ithreads(void);

/*! \brief Disables hardware interrupts.
 *
 * Calls to \fn intr_disable can nest, you must use the same number of calls to
 * \fn intr_enable to actually enable interrupts.
 *
 * In most scenarios threads should not switch out when interrupts are disabled.
 * However it should behave correctly, since context switch routine restores
 * interrupts state of target thread.
 *
 * If you need to disable preemption refer to \fn preempt_disable.
 *
 * \sa preempt_disable()
 */
void intr_disable(void);

/*! \brief Enables interrupts. */
void intr_enable(void);

/*! \brief Checks if interrupts are disabled now. */
bool intr_disabled(void);

/* Two following functions are workaround to make interrupt disabling work with
 * scoped and with statement. */
static inline void __intr_disable(void *data) {
  intr_disable();
}

static inline void __intr_enable(void *data) {
  intr_enable();
}

#define SCOPED_INTR_DISABLED()                                                 \
  SCOPED_STMT(void, __intr_disable, __intr_enable, NULL)

#define WITH_INTR_DISABLED WITH_STMT(void, __intr_disable, __intr_enable, NULL)

typedef struct intr_event intr_event_t;
typedef struct intr_handler intr_handler_t;

typedef enum {
  IF_STRAY = 0,    /* this device did not trigger the interrupt */
  IF_FILTERED = 1, /* the interrupt has been handled and can be EOId */
  IF_DELEGATE = 2  /* the handler should be run in private thread */
} intr_filter_t;

/*
 * The filter routine is run in primary interrupt context and may not
 * block or use regular mutexes.  The filter may either completely
 * handle the interrupt or it may perform some of the work and
 * defer more expensive work to the regular interrupt handler.
 */
typedef intr_filter_t ih_filter_t(void *);
typedef void ih_service_t(void *);
typedef void ie_action_t(intr_event_t *);

struct intr_handler {
  TAILQ_ENTRY(intr_handler) ih_link;
  ih_filter_t *ih_filter;   /* interrupt filter routine (run in irq ctx) */
  ih_service_t *ih_service; /* interrupt service routine (run in thread ctx) */
  intr_event_t *ih_event;   /* event we are connected to */
  void *ih_argument;        /* argument to pass to filter/service routines */
  const char *ih_name;      /* name of the handler */
  prio_t ih_prio;           /* handler's priority (sort key for ie_handlers) */
};

typedef TAILQ_HEAD(, intr_handler) ih_list_t;

#define INTR_HANDLER_INIT(filter, service, argument, desc, prio)               \
  (intr_handler_t) {                                                           \
    .ih_filter = (filter), .ih_service = (service), .ih_argument = (argument), \
    .ih_name = (desc), .ih_prio = (prio)                                       \
  }

/* Software representation of interrupt line. */
typedef struct intr_event {
  spin_t ie_lock;
  TAILQ_ENTRY(intr_event) ie_link; /* link on list of all interrupt events */
  ih_list_t ie_handlers;   /* interrupt handlers sorted by descending ih_prio */
  ie_action_t *ie_disable; /* called before ithread delegation (mask irq) */
  ie_action_t *ie_enable;  /* called after ithread delagation (unmask irq) */
  void *ie_source;         /* additional argument for actions */
  const char *ie_name;     /* individual event name */
  unsigned ie_irq;         /* physical interrupt request line number */
  unsigned ie_count;       /* number of handlers attached */
} intr_event_t;

typedef TAILQ_HEAD(, intr_event) ie_list_t;

void intr_event_init(intr_event_t *ie, unsigned irq, const char *name,
                     ie_action_t *disable, ie_action_t *enable, void *source);
void intr_event_register(intr_event_t *ie);
void intr_event_add_handler(intr_event_t *ie, intr_handler_t *ih);
void intr_event_remove_handler(intr_handler_t *ih);
void intr_event_run_handlers(intr_event_t *ie);

#endif /* !_SYS_INTERRUPT_H_ */
