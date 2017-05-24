#include <timer.h>

typedef TAILQ_HEAD(, timer_event) timer_event_list_t;
static timer_event_list_t events = TAILQ_HEAD_INITIALIZER(events);

int cpu_timer_add_event(timer_event_t *tev) {
  return 0;
}
int cpu_timer_remove_event(timer_event_t *tev) {
  return 0;
}
void cpu_timer_irq_handler() {
}
