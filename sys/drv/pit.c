/* Programable Interval Timer (PIT) driver for Intel 8253 */
#include <sys/mimiker.h>
#include <dev/i8253reg.h>
#include <dev/isareg.h>
#include <sys/pci.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/timer.h>
#include <sys/spinlock.h>
#include <sys/devclass.h>

#define PIIX4_IDE_VENDOR_ID 0x8086
#define PIIX4_IDE_DEVICE_ID 0x7111

typedef union {
  /* assumes little endian order */
  struct {
    uint32_t lo;
    uint32_t hi;
  };
  uint64_t val;
} counter_t;

typedef struct pit_state {
  resource_t *regs;
  intr_handler_t intr_handler;
  timer_t timer;
  volatile bintime_t time;
  volatile counter_t counter64_last;
  volatile uint16_t counter16_last;
  uint16_t period_cntr; /* period as PIT counter value */
  
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
  uint16_t counter16_now, ticks;
  uint32_t oldlow;

  counter16_now = pit_get_counter16(pit);
  if (pit->counter16_last >= counter16_now)
    ticks = pit->counter16_last - counter16_now;
  else
    ticks = pit->counter16_last + (pit->period_cntr - counter16_now);

  pit->counter16_last = counter16_now;

  oldlow = pit->counter64_last.lo;

  if (oldlow > pit->counter64_last.lo + ticks)
    pit->counter64_last.hi++;
  
  pit->counter64_last.lo += ticks;

  return pit->counter64_last.val;
}

static void pit_update_time(pit_state_t *pit) {
  uint64_t count = pit_get_counter64(pit);
  uint32_t freq = pit->timer.tm_frequency;
  uint32_t sec = count / freq;
  uint32_t frac = count % freq;
  bintime_t bt = bintime_mul(HZ2BT(freq), frac);
  bt.sec += sec;
  pit->time = bt;
}

static intr_filter_t pit_intr(void *data) {
  pit_state_t *pit = data;

  pit_update_time(pit);
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

  pit->time = BINTIME(0);
  pit->counter64_last.val = 0;
  pit->counter16_last = 0;
  pit->period_cntr = counter;

  pit_set_frequency(pit);

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

  return pit->time;
}

static int pit_attach(device_t *dev) {
  assert(dev->parent->bus == DEV_BUS_PCI);

  pit_state_t *pit = dev->state;

  pit->regs =
    bus_alloc_resource(dev, RT_IOPORTS, 0, IO_TIMER1,
                       IO_TIMER1 + IO_TMRSIZE - 1, IO_TMRSIZE, RF_ACTIVE);
  assert(pit->regs != NULL);

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

static int pit_probe(device_t *dev) {
  pci_device_t *pcid = pci_device_of(dev);
  return pci_device_match(pcid, PIIX4_IDE_VENDOR_ID, PIIX4_IDE_DEVICE_ID);
}

/* clang-format off */
static driver_t pit_driver = {
  .desc = "i8254 PIT driver",
  .size = sizeof(pit_state_t),
  .attach = pit_attach,
  .probe = pit_probe
};
/* clang-format on */

DEVCLASS_ENTRY(pci, pit_driver);
