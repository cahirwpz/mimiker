#define KL_LOG KL_TIME
#include <sys/klog.h>
#include <sys/timer.h>
#include <aarch64/armreg.h>
#include <aarch64/bcm2835reg.h>
#include <sys/interrupt.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/kmem.h>
#include <sys/pmap.h>

#define CNTCTL_ENABLE 1

/*
 * TODO(pj): This should be defined in one header for machine dependent AArch64
 * code.
 */
#define __isb() __asm__ volatile("ISB" ::: "memory")

typedef struct arm_timer_state {
  resource_t *rm_mem;
  vaddr_t va_page;
} arm_timer_state_t;

static int timer_start(timer_t *tm, unsigned flags, const bintime_t start,
                       const bintime_t period) {
  uint64_t step = bintime_mul(period, tm->tm_frequency).sec;

  WITH_INTR_DISABLED {
    uint64_t count = READ_SPECIALREG(cntpct_el0);
    WRITE_SPECIALREG(cntp_cval_el0, count + step);
    WRITE_SPECIALREG(cntp_ctl_el0, CNTCTL_ENABLE);

    __isb();
  }

  return 0;
}

static int timer_stop(timer_t *tm) {
  WRITE_SPECIALREG(cntp_ctl_el0, 0);
  return 0;
}

static bintime_t timer_gettime(timer_t *tm) {
  uint64_t count = READ_SPECIALREG(cntpct_el0);
  bintime_t val = bintime_mul(tm->tm_min_period, count);
  return val;
}

static intr_filter_t arm_timer_intr(void *data) {
  tm_trigger(data);

  /*
   * https://developer.arm.com/docs/ddi0595/h/aarch64-system-registers/cntp_cval_el0
   */
  uint64_t count = READ_SPECIALREG(cntpct_el0);
  uint64_t freq = READ_SPECIALREG(cntfrq_el0);
  WRITE_SPECIALREG(cntp_cval_el0, count + freq);

  __isb();

  return IF_FILTERED;
}

typedef struct timer_priv {
  device_t *dev;
  intr_handler_t intr_handler;
} timer_priv_t;

static timer_priv_t timer_priv;

static timer_t timer = (timer_t){
  .tm_name = "arm-cpu-timer",
  .tm_flags = TMF_PERIODIC,
  .tm_start = timer_start,
  .tm_stop = timer_stop,
  .tm_gettime = timer_gettime,
  .tm_priv = &timer_priv,
};

static int arm_timer_probe(device_t *dev) {
  return 1;
}

static int arm_timer_attach(device_t *dev) {
  /* Save link to timer device. */
  timer_priv_t *priv = timer.tm_priv;
  priv->dev = dev;
  priv->intr_handler =
    INTR_HANDLER_INIT(arm_timer_intr, NULL, &timer, "ARM CPU timer", 0);

  uint64_t freq = READ_SPECIALREG(cntfrq_el0);

  timer.tm_frequency = freq;
  timer.tm_min_period = HZ2BT(freq);
  /* TODO(pj): Document what this value means. */
  timer.tm_max_period = bintime_mul(HZ2BT(freq), 1LL << 30);
  tm_register(&timer);
  tm_select(&timer);

  arm_timer_state_t *state = dev->state;

  /* Request for shared memory. */
  state->rm_mem =
    bus_alloc_resource(dev, RT_MEMORY, 1, BCM2836_ARM_LOCAL_BASE,
                       BCM2836_ARM_LOCAL_BASE + BCM2836_ARM_LOCAL_SIZE - 1,
                       BCM2836_ARM_LOCAL_SIZE, RF_ACTIVE);

  assert(state->rm_mem != NULL);
  bus_space_map(state->rm_mem->r_bus_tag, state->rm_mem->r_start,
                state->rm_mem->r_end - state->rm_mem->r_start + 1,
                &state->rm_mem->r_bus_handle);

  state->va_page = state->rm_mem->r_bus_handle;

  /* Enable interrupt for CPU0. */
  size_t offset = BCM2836_LOCAL_TIMER_IRQ_CONTROLN(0);
  volatile uint32_t *timerp = (uint32_t *)(state->va_page + offset);
  *timerp = *timerp | (1 << BCM2836_INT_CNTPNSIRQ);

  bus_intr_setup(dev, BCM2836_INT_CNTPNSIRQ_CPUN(0), &priv->intr_handler);

  return 0;
}

static driver_t arm_timer = {
  .desc = "ARM CPU timer driver",
  .size = sizeof(arm_timer_state_t),
  .probe = arm_timer_probe,
  .attach = arm_timer_attach,
};

DEVCLASS_ENTRY(root, arm_timer);
