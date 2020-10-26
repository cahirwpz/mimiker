#define KL_LOG KL_TIME
#include <sys/klog.h>
#include <sys/timer.h>
#include <aarch64/armreg.h>
#include <aarch64/bcm2835reg.h>
#include <sys/interrupt.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/pmap.h>

#define CNTCTL_ENABLE 1

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

    /*
     * TODO(pj): This should be defined in one header for machine dependent
     * AArch64 code.
     */
    __asm__ volatile("isb" ::: "memory");
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

static timer_t timer = (timer_t){
  .tm_name = "arm-cpu-timer",
  .tm_flags = TMF_PERIODIC,
  .tm_start = timer_start,
  .tm_stop = timer_stop,
  .tm_gettime = timer_gettime,
};

static int arm_timer_attach(device_t *dev) {
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

  return 0;
}

void intr_tick(void) {
  tm_trigger(&timer);

  /*
   * https://developer.arm.com/docs/ddi0595/h/aarch64-system-registers/cntp_cval_el0
   */
  uint64_t count = READ_SPECIALREG(cntpct_el0);
  uint64_t freq = READ_SPECIALREG(cntfrq_el0);
  WRITE_SPECIALREG(cntp_cval_el0, count + freq);

  __asm__ volatile("isb" ::: "memory");
}

static driver_t arm_timer_driver = {
  .desc = "ARM CPU timer driver",
  .size = sizeof(arm_timer_state_t),
  .attach = arm_timer_attach,
};

void arm_timer_init(device_t *bus) {
  device_t *dev = device_add_child(bus, bus->devclass, -1);
  dev->driver = &arm_timer_driver;
  device_probe(dev);
  device_attach(dev);
}
