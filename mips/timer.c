#include <timer.h>
#include <interrupt.h>
#include <mips/m32c0.h>
#include <mips/config.h>
#include <mips/intr.h>
#include <sync.h>
#include <stdc.h>

static timer_event_list_t events = TAILQ_HEAD_INITIALIZER(events);

static inline uint32_t ticks(timer_event_t *tev) {
  return tev->tev_when.tv_sec * TICKS_PER_SEC +
         tev->tev_when.tv_usec * TICKS_PER_US;
}

timeval_t get_uptime(void) {
  /* BUG: C0_COUNT will overflow every 4294 seconds for 100MHz processor! */
  uint32_t count = mips32_get_c0(C0_COUNT) / TICKS_PER_US;
  return (timeval_t){.tv_sec = count / 1000000, .tv_usec = count % 1000000};
}

static intr_filter_t cpu_timer_intr(void *arg);

static intr_handler_t cpu_timer_intr_handler =
  INTR_HANDLER_INIT(cpu_timer_intr, NULL, NULL, "CPU timer", 0);

static intr_filter_t cpu_timer_intr(void *arg) {
  uint32_t compare = mips32_get_c0(C0_COMPARE);

  timer_event_t *event, *next;
  TAILQ_FOREACH_SAFE(event, &events, tev_link, next) {
    if (ticks(event) > compare)
      break;
    TAILQ_REMOVE(&events, event, tev_link);
    if (TAILQ_EMPTY(&events))
      mips_intr_teardown(&cpu_timer_intr_handler);
    assert(event->tev_func != NULL);
    event->tev_func(event);
  }

  if (!TAILQ_EMPTY(&events))
    mips32_set_c0(C0_COMPARE, ticks(TAILQ_FIRST(&events)));

  return IF_FILTERED;
}

void cpu_timer_add_event(timer_event_t *tev) {
  SCOPED_CRITICAL_SECTION();

  if (TAILQ_EMPTY(&events))
    mips_intr_setup(&cpu_timer_intr_handler, MIPS_HWINT5);

  timer_event_t *event;
  TAILQ_FOREACH (event, &events, tev_link)
    if (timeval_cmp(&event->tev_when, &tev->tev_when, >))
      break;

  if (event)
    TAILQ_INSERT_BEFORE(event, tev, tev_link);
  else
    TAILQ_INSERT_TAIL(&events, tev, tev_link);

  mips32_set_c0(C0_COMPARE, ticks(TAILQ_FIRST(&events)));
}

void cpu_timer_remove_event(timer_event_t *tev) {
  SCOPED_CRITICAL_SECTION();

  TAILQ_REMOVE(&events, tev, tev_link);

  if (TAILQ_EMPTY(&events))
    mips_intr_teardown(&cpu_timer_intr_handler);
  else
    mips32_set_c0(C0_COMPARE, ticks(TAILQ_FIRST(&events)));
}
