#define KL_LOG KL_TIME
#include <sys/devclass.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/timer.h>
#include <riscv/cpufunc.h>
#include <riscv/sbi.h>

typedef struct clint_state {
  timer_t mtimer;
  dev_intr_t *mswi_irq;
  dev_intr_t *mtimer_irq;
  uint64_t mtimer_step;
} clint_state_t;

/*
 * MTIMER device.
 */

static intr_filter_t mtimer_intr(void *data) {
  clint_state_t *clint = data;
  register_t sip = csr_read(sip);

  if (sip & SIP_STIP) {
    tm_trigger(&clint->mtimer);

    uint64_t prev = rdtime();
    sbi_set_timer(prev + clint->mtimer_step);

    return IF_FILTERED;
  }

  return IF_STRAY;
}

static int mtimer_start(timer_t *tm, unsigned flags, const bintime_t start,
                        const bintime_t period) {
  device_t *dev = tm->tm_priv;
  clint_state_t *clint = dev->state;
  clint->mtimer_step = bintime_mul(period, tm->tm_frequency).sec;

  int err =
    pic_setup_intr(dev, clint->mtimer_irq, mtimer_intr, NULL, clint, "MTIMER");
  assert(!err);

  WITH_INTR_DISABLED {
    uint64_t count = rdtime();
    sbi_set_timer(count + clint->mtimer_step);
  }

  return 0;
}

static int mtimer_stop(timer_t *tm) {
  device_t *dev = tm->tm_priv;
  clint_state_t *clint = dev->state;
  pic_teardown_intr(dev, clint->mtimer_irq);
  return 0;
}

static bintime_t mtimer_gettime(timer_t *tm) {
  uint64_t count = rdtime();
  bintime_t res = bintime_mul(tm->tm_min_period, (uint32_t)count);
  bintime_t high_bits = bintime_mul(tm->tm_min_period, (uint32_t)(count >> 32));
  bintime_add_frac(&res, high_bits.frac << 32);
  res.sec += (high_bits.sec << 32) + (high_bits.frac >> 32);
  return res;
}

/*
 * MSWI device.
 */

static intr_filter_t mswi_intr(void *data) {
  panic("Supervisor software interrupt!");
}

/*
 * CLINT driver interface.
 */

static int clint_probe(device_t *dev) {
  return FDT_is_compatible(dev->node, "riscv,clint0");
}

static int clint_attach(device_t *dev) {
  clint_state_t *clint = dev->state;
  int err = 0;

  phandle_t cpus = FDT_finddevice("/cpus");
  if (cpus == FDT_NODEV)
    return ENXIO;

  uint32_t freq;
  if (FDT_getencprop(cpus, "timebase-frequency", (void *)&freq,
                     sizeof(uint32_t)) != sizeof(uint32_t))
    return ENXIO;

  clint->mswi_irq = device_take_intr(dev, 2);
  assert(clint->mswi_irq);

  clint->mtimer_irq = device_take_intr(dev, 3);
  assert(clint->mtimer_irq);

  if ((err =
         pic_setup_intr(dev, clint->mswi_irq, mswi_intr, NULL, NULL, "SSI"))) {
    return (err == ENODEV) ? EAGAIN : err;
  }

  clint->mtimer = (timer_t){
    .tm_name = "RISC-V CLINT",
    .tm_flags = TMF_PERIODIC,
    .tm_frequency = freq,
    .tm_min_period = HZ2BT(freq),
    .tm_max_period = bintime_mul(HZ2BT(freq), (1LL << 32) - 1),
    .tm_start = mtimer_start,
    .tm_stop = mtimer_stop,
    .tm_gettime = mtimer_gettime,
    .tm_priv = dev,
  };

  tm_register(&clint->mtimer);

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
