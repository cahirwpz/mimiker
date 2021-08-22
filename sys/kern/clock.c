#define KL_LOG KL_TIME
#include <sys/callout.h>
#include <sys/sched.h>
#include <sys/mimiker.h>
#include <sys/klog.h>
#include <sys/timer.h>
#include <sys/kgprof.h>

static systime_t now = 0;
static timer_t *clock = NULL;
static timer_t *profclock = NULL;

systime_t getsystime(void) {
  return now;
}

static void prof_clock(timer_t *tm, void *arg) {
  kgprof_tick();
}

static void clock_cb(timer_t *tm, void *arg) {
  bintime_t bin = binuptime();
  now = bt2st(&bin);
  if (profclock == NULL)
    prof_clock(tm, arg);
  callout_process(now);
  sched_clock();
}

void init_clock(void) {
  clock = tm_reserve(NULL, TMF_PERIODIC);
  profclock = get_stat_timer();
  if (clock == NULL)
    panic("Missing suitable timer for maintenance of system clock!");

  set_kgprof_profrate(CLK_TCK);
  tm_init(clock, clock_cb, NULL);
  if (tm_start(clock, TMF_PERIODIC | TMF_TIMESOURCE, (bintime_t){},
               HZ2BT(CLK_TCK)))
    panic("Failed to start system clock!");
  klog("System clock uses \'%s\' hardware timer.", clock->tm_name);

  if (profclock != NULL) {
    tm_init(profclock, prof_clock, NULL);
    set_kgprof_profrate(PROF_TCK);
    if (tm_start(profclock, TMF_PERIODIC, (bintime_t){}, HZ2BT(PROF_TCK))) {
      klog("Failed to start profclock!");
      tm_release(profclock);
      set_kgprof_profrate(CLK_TCK);
      profclock = NULL;
    } else
      klog("Statclock uses \'%s\' hardware timer.", profclock->tm_name);
  }
}
