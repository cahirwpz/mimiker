#include <stdc.h>
#include <sched.h>
#include <runq.h>
#include <context.h>
#include <clock.h>
#include <thread.h>
#include <callout.h>
#include <interrupts.h>
#include <mutex.h>

static runq_t runq;
static thread_t *td_sched;
static callout_t sched_callout;

void sched_init() {
  runq_init(&runq);
}

void sched_add(thread_t *td) {
  log("Add '%s' {%p} thread to scheduler", td->td_name, td);

  intr_disable();
  runq_add(&runq, td);
  intr_enable();
}

static bool sched_activate = false;

void sched_yield() {
  thread_t *td = thread_self();

  assert(td != td_sched);

  callout_stop(&sched_callout);
  runq_add(&runq, td);
  thread_switch_to(td_sched);
}

static void sched_wakeup() {
  thread_t *td = thread_self();

  assert(td != td_sched);

  callout_stop(&sched_callout);
  runq_add(&runq, td);
  sched_activate = true;
}

void sched_resume() {
  bool irq_active = mips32_get_c0(C0_STATUS) & SR_EXL;
  assert(irq_active);

  if (sched_activate) {
    sched_activate = false;
    mips32_bc_c0(C0_STATUS, SR_EXL);
    intr_enable();
    thread_switch_to(td_sched);
  }
}

noreturn void sched_run(size_t quantum) {
  td_sched = thread_self();

  while (true) {
    thread_t *td;

    while (!(td = runq_choose(&runq)));

    runq_remove(&runq, td);
    callout_setup(&sched_callout, clock_get_ms() + quantum, sched_wakeup, NULL);
    thread_switch_to(td);
  }
}
