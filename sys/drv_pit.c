#include <common.h>
#include <dev/i8253reg.h>
#include <dev/isareg.h>
#include <pci.h>
#include <interrupt.h>
#include <klog.h>
#include <spinlock.h>
#include <sysinit.h>

typedef struct pit_state {
  resource_t *regs;
  spinlock_t lock;
  uint32_t counter;
  intr_handler_t intr_handler;
} pit_state_t;

#define inb(addr) bus_space_read_1(pit->regs, IO_TIMER1 + (addr))
#define outb(addr, val) bus_space_write_1(pit->regs, IO_TIMER1 + (addr), (val))

static void pit_set_frequency(pit_state_t *pit, unsigned freq) {
  uint16_t period = TIMER_DIV(freq);

  WITH_SPINLOCK(&pit->lock) {
    outb(TIMER_MODE, TIMER_SEL0 | TIMER_16BIT | TIMER_RATEGEN);
    outb(TIMER_CNTR0, period & 0xff);
    outb(TIMER_CNTR0, period >> 8);
  }
}

static uint16_t pit_get_counter(pit_state_t *pit) {
  uint16_t val = 0;

  WITH_SPINLOCK(&pit->lock) {
    outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
    val = inb(TIMER_CNTR0);
    val |= inb(TIMER_CNTR0) << 8;
  }

  return val;
}

static void pit_set_counter(pit_state_t *pit, uint16_t value) {
  WITH_SPINLOCK(&pit->lock) {
    outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
    outb(TIMER_CNTR0, value & 0xff);
    outb(TIMER_CNTR0, value >> 8);
  }
}

static intr_filter_t pit_intr(void *data) {
  pit_state_t *pit = data;
  klog("PIT: %d", pit_get_counter(pit));
  return IF_FILTERED;
}

static int pit_attach(device_t *dev) {
  assert(dev->parent->bus == DEV_BUS_PCI);

  pci_bus_state_t *pcib = dev->parent->state;
  pit_state_t *pit = dev->state;

  pit->regs = pcib->io_space;

  pit->lock = SPINLOCK_INITIALIZER();
  pit->intr_handler =
    INTR_HANDLER_INIT(pit_intr, NULL, pit, "programmable interval timer", 0);
  bus_intr_setup(dev, 0, &pit->intr_handler);

  return 0;
}

static driver_t pit_driver = {
  .desc = "i8253 PIT driver", .size = sizeof(pit_state_t), .attach = pit_attach,
};

extern device_t *gt_pci;

static void pit_init(void) {
  device_t *dev = make_device(gt_pci, &pit_driver);
  pit_state_t *pit = dev->state;
  pit_set_counter(pit, 0);
  pit_set_frequency(pit, 100);
}

SYSINIT_ADD(pit, pit_init, DEPS("rootdev"));
