/* Programmable Interval Timer (PIT) driver for Intel 8253 */
#include <sys/mimiker.h>
#include <dev/i8253reg.h>
#include <dev/isareg.h>
#include <sys/bus.h>
#include <sys/interrupt.h>
#include <sys/timer.h>
#include <sys/devclass.h>

typedef struct pit_state {
  resource_t *regs;
  resource_t *irq_res;
  timer_t timer;
  uint16_t period_cntr;      /* period as PIT counter value */
  uint16_t cntr16_prev_read; /* last read counter value */
  timercntr_t cntr64;        /* counter value since timer initialization */
} pit_state_t;

#define inb(addr) bus_read_1(pit->regs, (addr))
#define outb(addr, val) bus_write_1(pit->regs, (addr), (val))

static void pit_set_frequency(pit_state_t *pit) {
  outb(TIMER_MODE, TIMER_SEL0 | TIMER_16BIT | TIMER_RATEGEN);
  outb(TIMER_CNTR0, pit->period_cntr & 0xff);
  outb(TIMER_CNTR0, pit->period_cntr >> 8);
}

static uint16_t pit_get_counter16(pit_state_t *pit) {
  uint16_t count = 0;
  outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
  count |= inb(TIMER_CNTR0);
  count |= inb(TIMER_CNTR0) << 8;
  return count;
}

static uint64_t pit_get_counter64(pit_state_t *pit) {
  SCOPED_INTR_DISABLED();
  uint16_t cntr16_now = pit_get_counter16(pit);
  /* PIT counter counts from n to 1 and when we get to 1 an interrupt
     is send and the counter starts from the beginning (n = pit->period_cntr).
     We do not guarantee that we will not miss the whole period (n ticks) */
  uint16_t ticks = pit->cntr16_prev_read - cntr16_now;
  if (pit->cntr16_prev_read < cntr16_now)
    ticks += pit->period_cntr;

  /* We want to keep the last read counter value to detect possible future
   * overflows of our counter */
  pit->cntr16_prev_read = cntr16_now;

  uint32_t oldlow = pit->cntr64.lo;
  pit->cntr64.lo += ticks;
  if (oldlow > oldlow + ticks)
    pit->cntr64.hi++;

  return pit->cntr64.val;
}

static intr_filter_t pit_intr(void *data) {
  pit_state_t *pit = data;

  /* XXX: It's still possible for a tick to be lost. */
  (void)pit_get_counter64(pit);
  tm_trigger(&pit->timer);
  return IF_FILTERED;
}

static device_t *device_of(timer_t *tm) {
  return tm->tm_priv;
}

static int timer_pit_start(timer_t *tm, unsigned flags, const bintime_t start,
                           const bintime_t period) {
  assert(flags & TMF_PERIODIC);
  assert(!(flags & TMF_ONESHOT));

  device_t *dev = device_of(tm);
  pit_state_t *pit = dev->state;

  uint32_t counter = bintime_mul(period, TIMER_FREQ).sec;
  /* Maximal counter value which we can store in pit timer */
  assert(counter <= 0xFFFF);

  pit->cntr64.val = 0;
  pit->cntr16_prev_read = 0;
  pit->period_cntr = counter;

  pit_set_frequency(pit);

  bus_intr_setup(dev, pit->irq_res, pit_intr, NULL, pit, "i8254 timer");
  return 0;
}

static int timer_pit_stop(timer_t *tm) {
  device_t *dev = device_of(tm);
  pit_state_t *pit = dev->state;
  bus_intr_teardown(dev, pit->irq_res);
  return 0;
}

static bintime_t timer_pit_gettime(timer_t *tm) {
  device_t *dev = device_of(tm);
  pit_state_t *pit = dev->state;
  uint64_t count = pit_get_counter64(pit);
  uint32_t freq = pit->timer.tm_frequency;
  uint32_t sec = count / freq;
  uint32_t frac = count % freq;
  bintime_t bt = bintime_mul(HZ2BT(freq), frac);
  bt.sec += sec;
  return bt;
}

static int pit_attach(device_t *dev) {
  pit_state_t *pit = dev->state;

  pit->regs = device_take_ioports(dev, 0, RF_ACTIVE);
  assert(pit->regs != NULL);

  pit->irq_res = device_take_irq(dev, 0, RF_ACTIVE);

  pit->timer = (timer_t){
    .tm_name = "i8254",
    .tm_flags = TMF_PERIODIC,
    .tm_quality = 100,
    .tm_frequency = TIMER_FREQ,
    .tm_min_period = HZ2BT(TIMER_FREQ),
    .tm_max_period = bintime_mul(HZ2BT(TIMER_FREQ), 65536),
    .tm_start = timer_pit_start,
    .tm_stop = timer_pit_stop,
    .tm_gettime = timer_pit_gettime,
    .tm_priv = dev,
  };

  tm_register(&pit->timer);

  return 0;
}

static int pit_probe(device_t *dev) {
  return dev->unit == 3; /* XXX: unit 3 assigned by gt_pci */
}

static driver_t pit_driver = {
  .desc = "i8254 PIT driver",
  .size = sizeof(pit_state_t),
  .pass = FIRST_PASS,
  .attach = pit_attach,
  .probe = pit_probe,
};

DEVCLASS_ENTRY(isa, pit_driver);
