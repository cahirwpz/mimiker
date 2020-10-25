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
  .tm_priv = NULL,
};

void init_arm_timer(void) {
  uint64_t freq = READ_SPECIALREG(cntfrq_el0);

  timer.tm_frequency = freq;
  timer.tm_min_period = HZ2BT(freq);
  /* TODO(pj): Document what this value means. */
  timer.tm_max_period = bintime_mul(HZ2BT(freq), 1LL << 30);
  tm_register(&timer);
  tm_select(&timer);
}

static vaddr_t va;

int arm_timer_attach(device_t *dev) {
  /* TODO(pj): This memory should be managed by bus_space_map. */
  va = kva_alloc(PAGESIZE);
  assert((void *)va != NULL);

  paddr_t pa = BCM2836_ARM_LOCAL_BASE;
  pmap_kenter(va, pa, VM_PROT_READ | VM_PROT_WRITE, PMAP_NOCACHE);

  size_t offset = BCM2836_LOCAL_TIMER_IRQ_CONTROLN(0);
  volatile uint32_t *timerp = (uint32_t *)(va + offset);
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
