#ifndef _SYS_INTERRUPT_H_
#define _SYS_INTERRUPT_H_

#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/priority.h>

#define IENAMELEN 32

typedef struct ctx ctx_t;
typedef struct device device_t;
typedef struct resource resource_t;
typedef struct fdt_intr fdt_intr_t;
typedef uint32_t pcell_t;

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
void intr_disable(void) __no_profile;

/*! \brief Enables interrupts. */
void intr_enable(void) __no_profile;

/*! \brief Checks if interrupts are disabled now. */
bool intr_disabled(void) __no_profile;

/* Two following functions are workaround to make interrupt disabling work with
 * scoped and with statement. */
static __no_profile inline void __intr_disable(void *data) {
  intr_disable();
}

static __no_profile inline void __intr_enable(void *data) {
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
typedef intr_filter_t ih_filter_t(void *);
typedef void ih_service_t(void *);
typedef void ie_action_t(intr_event_t *);

/* Software representation of interrupt line. */
typedef struct intr_event {
  mtx_t ie_lock;
  TAILQ_ENTRY(intr_event) ie_link; /* link on list of all interrupt events */
  TAILQ_HEAD(, intr_handler) ie_handlers; /* sorted by descending ih_prio */
  ie_action_t *ie_disable; /* called before ithread delegation (mask irq) */
  ie_action_t *ie_enable;  /* called after ithread delagation (unmask irq) */
  void *ie_source;         /* additional argument for actions */
  char ie_name[IENAMELEN]; /* individual event name */
  unsigned ie_irq;         /* physical interrupt request line number */
  thread_t *ie_ithread;    /* associated interrupt thread */
} intr_event_t;

intr_event_t *intr_event_create(void *source, int irq, ie_action_t *disable,
                                ie_action_t *enable, const char *name);
intr_handler_t *intr_event_add_handler(intr_event_t *ie, ih_filter_t *filter,
                                       ih_service_t *service, void *arg,
                                       const char *name);
void intr_event_remove_handler(intr_handler_t *ih);
void intr_event_run_handlers(intr_event_t *ie);

typedef void (*intr_root_filter_t)(ctx_t *ctx, device_t *dev);

void intr_root_claim(intr_root_filter_t filter, device_t *dev);
void intr_root_handler(ctx_t *ctx) __no_profile;

/*
 * Interrupt controller interface.
 */

typedef void (*pic_setup_intr_t)(device_t *pic, device_t *dev, resource_t *r,
                                 ih_filter_t *filter, ih_service_t *service,
                                 void *arg, const char *name);
typedef void (*pic_teardown_intr_t)(device_t *pic, device_t *dev,
                                    resource_t *r);
typedef int (*pic_map_intr_t)(device_t *pic, device_t *dev, pcell_t *intr,
                              int icells);

typedef struct pic_methods {
  pic_setup_intr_t setup_intr;
  pic_teardown_intr_t teardown_intr;
  pic_map_intr_t map_intr;
} pic_methods_t;

/*
 * Register a new interrupt source for interrupt identified by `irq`.
 *
 * Arguments:
 *  - `dev`: requesting device
 *  - `irq`: interrupt resource
 *  - `filter`: filter function called within interrupted context
 *  - `service`: optional service function called within interrupt
 *    thread context
 *  - `arg`: argument passed to both filter and service routines
 *  - `name`: description of the interrupt source
 */
void pic_setup_intr(device_t *dev, resource_t *irq, ih_filter_t *filter,
                    ih_service_t *service, void *arg, const char *name);

/*
 * Remove specified interrupt source.
 *
 * Arguments:
 *  - `dev`: requesting device
 *  - `r`: interrupt resource
 */
void pic_teardown_intr(device_t *dev, resource_t *r);

/*
 * Map FDT interrupt resource of device `dev`
 * into an interrupt controller-specific interrupt number.
 *
 * Returns:
 *  - >= 0: PIC-specific interrupt number to identify PIC interrupt resource
 *  - -1: the FDT interrupt resource is invalid
 */
int pic_map_intr(device_t *dev, fdt_intr_t *intr);

#endif /* !_SYS_INTERRUPT_H_ */
