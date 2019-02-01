#ifndef _SYS_INTERRUPT_H_
#define _SYS_INTERRUPT_H_

#include <common.h>
#include <queue.h>
#include <spinlock.h>
#include <priority.h>

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
  IF_DELEGATE = 2, /* the handler should be run in private thread */
} intr_filter_t;

/*
 * The filter routine is run in primary interrupt context and may not
 * block or use regular mutexes.  The filter may either completely
 * handle the interrupt or it may perform some of the work and
 * defer more expensive work to the regular interrupt handler.
 */
typedef intr_filter_t driver_filter_t(void *);
typedef void driver_intr_t(void *);
typedef void driver_mask_t(intr_event_t *);

struct intr_handler {
  TAILQ_ENTRY(intr_handler) ih_list;
  driver_filter_t *ih_filter; /* driver interrupt filter function */
  driver_intr_t *ih_handler;  /* driver interrupt handler function */
  intr_event_t *ih_event;     /* event we are connected to */
  void *ih_argument;          /* argument to pass to the handler */
  char *ih_name;              /* name of the handler */
  prio_t ih_prio;             /* priority of the handler */
};

typedef TAILQ_HEAD(, intr_handler) intr_handler_list_t;

#define INTR_HANDLER_INIT(filter, handler, argument, desc, prio)               \
  (intr_handler_t) {                                                           \
    .ih_filter = (filter), .ih_handler = (handler), .ih_argument = (argument), \
    .ih_name = (desc), .ih_prio = (prio)                                       \
  }

typedef struct intr_event {
  spin_t ie_lock;
  TAILQ_ENTRY(intr_event) ie_list;
  intr_handler_list_t ie_handlers; /* interrupt handlers */
  driver_mask_t *ie_mask_irq;      /* called before ithread delegation */
  driver_mask_t *ie_unmask_irq;    /* called after ithread delagation */
  void *ie_source;                 /* additional argument for mask and unmask */
  const char *ie_name;             /* individual event name */
  unsigned ie_irq;                 /* physical interrupt request line number */
  unsigned ie_count;               /* number of handlers attached */
} intr_event_t;

typedef TAILQ_HEAD(, intr_event) intr_event_list_t;

void intr_thread(void *arg);
void intr_event_init(intr_event_t *ie, unsigned irq, const char *name,
                     driver_mask_t *mask_irq, driver_mask_t *unmask_irq,
                     void *source);
void intr_event_register(intr_event_t *ie);
void intr_event_add_handler(intr_event_t *ie, intr_handler_t *ih);
void intr_event_remove_handler(intr_handler_t *ih);
void intr_event_run_handlers(intr_event_t *ie);

#endif /* _SYS_INTERRUPT_H_ */
