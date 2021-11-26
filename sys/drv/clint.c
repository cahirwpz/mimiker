#define KL_LOG KL_TIME
#include <sys/devclass.h>
#include <sys/dtb.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/timer.h>
#include <riscv/riscvreg.h>
#include <riscv/sbi.h>

typedef struct clint_state {
  timer_t timer;
  resource_t *software_irq;
  resource_t *timer_irq;
  uint64_t timer_step;
} clint_state_t;

/*
 * CLINT timer.
 */

static intr_filter_t clint_timer_intr(void *data) {
  clint_state_t *clint = data;
  register_t sip = csr_read(sip);

  if (sip & SIP_STIP) {
    sbi_set_timer(rdtime() + clint->timer_step);
    csr_clear(sip, SIP_STIP);
    return IF_FILTERED;
  }

  return IF_FILTERED;
}

static int clint_timer_start(timer_t *tm, unsigned flags, const bintime_t start,
                             const bintime_t period) {
  device_t *dev = tm->tm_priv;
  clint_state_t *clint = dev->state;

  clint->timer_step = bintime_mul(period, tm->tm_frequency).sec;

  intr_setup(dev, clint->timer_irq, clint_timer_intr, NULL, clint,
             "CLINT timer");

  sbi_set_timer(rdtime() + clint->timer_step);
  return 0;
}

static int clint_timer_stop(timer_t *tm) {
  device_t *dev = tm->tm_priv;
  clint_state_t *clint = dev->state;
  intr_teardown(dev, clint->timer_irq);
  return 0;
}

static bintime_t clint_timer_gettime(timer_t *tm) {
  uint64_t count = rdtime();
  bintime_t res = bintime_mul(tm->tm_min_period, (uint32_t)count);
  bintime_t high_bits = bintime_mul(tm->tm_min_period, (uint32_t)(count >> 32));
  bintime_add_frac(&res, high_bits.frac << 32);
  res.sec += (high_bits.sec << 32) + (high_bits.frac >> 32);
  return res;
}

/*
 * CLINT driver interface.
 */

static int clint_probe(device_t *dev) {
  void *dtb = dtb_root();
  const char *compatible = fdt_getprop(dtb, dev->node, "compatible", NULL);
  if (!compatible)
    return 0;
  return strcmp(compatible, "riscv,clint0") == 0;
}

static uint32_t clint_frequency(void) {
  void *dtb = dtb_root();
  int cpus_node = dtb_offset("cpus");
  int len;

  const uint32_t *prop =
    fdt_getprop(dtb, cpus_node, "timebase-frequency", &len);
  if (!prop || len != sizeof(uint32_t))
    panic("Invalid timebase-frequency property!");

  return fdt32_to_cpu(*prop);
}

static int clint_attach(device_t *dev) {
  clint_state_t *clint = dev->state;

  clint->software_irq = device_take_irq(dev, 0, RF_ACTIVE);
  assert(clint->software_irq);

  clint->timer_irq = device_take_irq(dev, 1, RF_ACTIVE);
  assert(clint->timer_irq);

  uint32_t freq = clint_frequency();

  clint->timer = (timer_t){
    .tm_name = "RISC-V CLINT",
    .tm_flags = TMF_PERIODIC,
    .tm_frequency = freq,
    .tm_min_period = HZ2BT(freq),
    .tm_max_period = bintime_mul(HZ2BT(freq), (1LL << 32) - 1),
    .tm_start = clint_timer_start,
    .tm_stop = clint_timer_stop,
    .tm_gettime = clint_timer_gettime,
    .tm_priv = dev,
  };

  tm_register(&clint->timer);

  return 0;
}

static driver_t clint_driver = {
  .desc = "RISC-V CLINT driver",
  .size = sizeof(clint_state_t),
  .pass = FIRST_PASS,
  .probe = clint_probe,
  .attach = clint_attach,
};

DEVCLASS_ENTRY(root, clint_driver);
