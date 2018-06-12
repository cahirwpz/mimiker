#include <common.h>
#include <dev/i8253reg.h>
#include <dev/isareg.h>
#include <pci.h>
#include <interrupt.h>
#include <klog.h>
#include <timer.h>
#include <spinlock.h>
#include <sysinit.h>

typedef struct pit_state {
  resource_t *regs;
  spinlock_t lock;
  intr_handler_t intr_handler;
  timer_t timer;
  uint64_t tick;
  uint16_t period;
  bintime_t time;
} pit_state_t;

#define inb(addr) bus_space_read_1(pit->regs, IO_TIMER1 + (addr))
#define outb(addr, val) bus_space_write_1(pit->regs, IO_TIMER1 + (addr), (val))

static void pit_set_frequency(pit_state_t *pit, uint16_t period) {
  assert(spin_owned(&pit->lock));

  outb(TIMER_MODE, TIMER_SEL0 | TIMER_16BIT | TIMER_RATEGEN);
  outb(TIMER_CNTR0, period & 0xff);
  outb(TIMER_CNTR0, period >> 8);
  pit->period = period;
}

static uint16_t pit_get_counter(pit_state_t *pit) {
  assert(spin_owned(&pit->lock));

  outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
  uint16_t value = 0;
  value |= inb(TIMER_CNTR0);
  value |= inb(TIMER_CNTR0) << 8;
  return value;
}

static intr_filter_t pit_intr(void *data) {
  pit_state_t *pit = data;
  bintime_add_frac(&pit->time, pit->tick);
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

  pit->time = start;
  pit->tick = period.frac;
  uint16_t counter = bintime_mul(period, TIMER_FREQ).sec;
  WITH_SPINLOCK(&pit->lock) {
    pit_set_frequency(pit, counter);
  }
  bus_intr_setup(dev, 0, &pit->intr_handler);
  return 0;
}

static int timer_pit_stop(timer_t *tm) {
  device_t *dev = device_of(tm);
  pit_state_t *pit = dev->state;

  bus_intr_teardown(dev, &pit->intr_handler);
  return 0;
}

static bintime_t timer_pit_gettime(timer_t *tm) {
  device_t *dev = device_of(tm);
  pit_state_t *pit = dev->state;
  uint16_t counter = pit->period;
  bintime_t bt;
  bintime_t offset;
  WITH_SPINLOCK(&pit->lock) {
    bt = pit->time;
    counter -= pit_get_counter(pit);
  }
  offset = bintime_mul(tm->tm_min_period, counter);
  bintime_add_frac(&bt, offset.frac);
  return bt;
}

static int pit_attach(device_t *dev) {
  assert(dev->parent->bus == DEV_BUS_PCI);

  pit_state_t *pit = dev->state;

  pit->regs = bus_resource_alloc_anywhere(dev, RT_ISA_F | RT_IOPORTS, 0, 0, RF_SHARED);

  pit->lock = SPINLOCK_INITIALIZER();
  pit->intr_handler = INTR_HANDLER_INIT(pit_intr, NULL, pit, "i8254 timer", 0);

  pit->timer = (timer_t){
    .tm_name = "i8254",
    .tm_flags = TMF_PERIODIC,
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

static driver_t pit_driver = {
  .desc = "i8254 PIT driver", .size = sizeof(pit_state_t), .attach = pit_attach,
};

extern device_t *gt_pci;

static void pit_init(void) {
  (void)make_device(gt_pci, &pit_driver);
}

SYSINIT_ADD(pit, pit_init, DEPS("rootdev"));
