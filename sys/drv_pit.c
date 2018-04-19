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
  uint32_t ticks;
  uint16_t period;
  intr_handler_t intr_handler;
  timer_t timer;
} pit_state_t;

#define inb(addr) bus_space_read_1(pit->regs, IO_TIMER1 + (addr))
#define outb(addr, val) bus_space_write_1(pit->regs, IO_TIMER1 + (addr), (val))

static void pit_set_frequency(pit_state_t *pit, unsigned freq) {
  uint16_t period = TIMER_DIV(freq);

  WITH_SPINLOCK(&pit->lock) {
    outb(TIMER_MODE, TIMER_SEL0 | TIMER_16BIT | TIMER_RATEGEN);
    outb(TIMER_CNTR0, period & 0xff);
    outb(TIMER_CNTR0, period >> 8);

    pit->period = period;
  }
}

#if 0
static uint16_t pit_get_counter(pit_state_t *pit) {
  uint16_t val = 0;

  WITH_SPINLOCK(&pit->lock) {
    outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
    val = inb(TIMER_CNTR0);
    val |= inb(TIMER_CNTR0) << 8;
  }

  return val;
}
#endif

static void pit_set_counter(pit_state_t *pit, uint16_t value) {
  WITH_SPINLOCK(&pit->lock) {
    outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
    outb(TIMER_CNTR0, value & 0xff);
    outb(TIMER_CNTR0, value >> 8);
  }
}

static intr_filter_t pit_intr(void *data) {
  pit_state_t *pit = data;
  tm_trigger(&pit->timer);
  return IF_FILTERED;
}

static device_t *device_of(timer_t *tm) {
  return tm->tm_priv;
}

static bintime_t timer_pit_gettime(timer_t *tm) {
  pit_state_t *pit = device_of(tm)->state;

  /* TODO add offset based on current value of PIT counter */
  return bintime_mul(bintime_mul(HZ2BT(TIMER_FREQ), pit->period), pit->ticks);
}

static int timer_pit_start(timer_t *tm, unsigned flags, const bintime_t value) {
  assert(flags & TMF_PERIODIC);
  assert(!(flags & TMF_ONESHOT));

  device_t *dev = device_of(tm);
  pit_state_t *pit = dev->state;

  bintime_t period = bintime_mul(value, TIMER_FREQ);
  pit_set_frequency(pit, period.sec);

  bus_intr_setup(dev, 0, &pit->intr_handler);
  pit_set_counter(pit, 0);
  return 0;
}

static int timer_pit_stop(timer_t *tm) {
  device_t *dev = device_of(tm);
  pit_state_t *pit = dev->state;

  bus_intr_teardown(dev, &pit->intr_handler);
  return 0;
}

static int pit_attach(device_t *dev) {
  assert(dev->parent->bus == DEV_BUS_PCI);

  pci_bus_state_t *pcib = dev->parent->state;
  pit_state_t *pit = dev->state;

  pit->regs = pcib->io_space;

  pit->lock = SPINLOCK_INITIALIZER();
  pit->intr_handler = INTR_HANDLER_INIT(pit_intr, NULL, pit, "i8254 timer", 0);

  pit->timer = (timer_t){
    .tm_name = "i8254",
    .tm_flags = TMF_PERIODIC,
    .tm_frequency = TIMER_FREQ,
    .tm_min_period = HZ2BT(TIMER_FREQ),
    .tm_max_period = bintime_mul(HZ2BT(TIMER_FREQ), 65536),
    .tm_gettime = timer_pit_gettime,
    .tm_start = timer_pit_start,
    .tm_stop = timer_pit_stop,
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
