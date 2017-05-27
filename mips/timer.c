#include <timer.h>
#include <interrupt.h>
#include <mips/m32c0.h>
#include <mips/config.h>
#include <mips/intr.h>
#include <mips/clock.h>
#include <sysinit.h>
#include <malloc.h>

typedef TAILQ_HEAD(, timer_event) timer_event_list_t;
static timer_event_list_t events = TAILQ_HEAD_INITIALIZER(events);

int cpu_timer_add_event(timer_event_t *tev) {
  //NOT READY
  TAILQ_INSERT_HEAD(&events, tev, tev_link);
  mips32_set_c0(C0_COMPARE, tv2tk(tev->tev_when));
  return 0;
}

int cpu_timer_remove_event(timer_event_t *tev) {
  timer_event_t *event;
  TAILQ_FOREACH (event, &events, tev_link)
    if (event == tev) {
      TAILQ_REMOVE(&events, event, tev_link);
      break;
    }
  return 0;
}

void cpu_timer_intr(void *arg) {
  uint32_t compare = mips32_get_c0(C0_COMPARE);
  uint32_t count = mips32_get_c0(C0_COUNT);
  /* Should not happen. Potentially spurious interrupt. */
  if (compare - count > 0)
    return;

  struct timer_event *event;
  TAILQ_FOREACH (event, &events, tev_link) {
    if (tv2tk(event->tev_when) > compare) {
      mips32_set_c0(C0_COMPARE, tv2tk(event->tev_when));
      break;
    }
    TAILQ_REMOVE(&events, event, tev_link);
    event->tev_func(event);
  }
}

static INTR_HANDLER_DEFINE(cpu_timer_intr_handler, NULL, cpu_timer_intr, NULL,
                           "CPU timer", 0);

static void cpu_timer_init(void) {
  mips32_set_c0(C0_COUNT, 0);
  mips32_set_c0(C0_COMPARE, TICKS_PER_MS);
  mips_intr_setup(cpu_timer_intr_handler, 7);
  timer_event_t *clock = kmalloc(M_TEMP, sizeof(*clock), M_ZERO);
  clock->tev_when = TIMEVAL(0.001);
  clock->tev_func = mips_clock;
  cpu_timer_add_event(clock);
}

SYSINIT_ADD(cpu_timer, cpu_timer_init, DEPS("callout", "sched"));
