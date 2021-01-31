#define KL_LOG KL_TIME
#include <sys/callout.h>
#include <sys/sched.h>
#include <sys/mimiker.h>
#include <sys/klog.h>
#include <sys/timer.h>
#include <sys/context.h>
#include <sys/gmon.h>

static systime_t now = 0;
static timer_t *clock = NULL;

systime_t getsystime(void) {
  return now;
}

#if KPROF
static void statclock(void) {
  gmonparam_t *g = &_gmonparam;
  thread_t *td = thread_self();
  klog("Got thread - ctx %d.", td->td_kframe);
  if (td->td_kframe == NULL)
    return;
  uintptr_t pc = ctx_get_pc(td->td_kframe), instr;
  klog("Got pc register.");
  if (g->state == GMON_PROF_ON && pc >= g->lowpc) {
    klog("Gmon working.");
    instr = pc - g->lowpc;
    if (instr < g->textsize) {
      klog("Gmon collecting stats.");
      instr /= HISTFRACTION * sizeof(*g->kcount);
      g->kcount[instr]++;
      klog("Gmon updated instr %d kcountsize %d.", instr, g->kcountsize);
    }
  }
}
#else
static void statclock(void) {
}
#endif

static void clock_cb(timer_t *tm, void *arg) {
  bintime_t bin = binuptime();
  now = bt2st(&bin);
  statclock();
  callout_process(now);
  sched_clock();
}

void init_clock(void) {
  clock = tm_reserve(NULL, TMF_PERIODIC);
  if (clock == NULL)
    panic("Missing suitable timer for maintenance of system clock!");
  tm_init(clock, clock_cb, NULL);
  if (tm_start(clock, TMF_PERIODIC | TMF_TIMESOURCE, (bintime_t){},
               HZ2BT(CLK_TCK)))
    panic("Failed to start system clock!");
  klog("System clock uses \'%s\' hardware timer.", clock->tm_name);
}
