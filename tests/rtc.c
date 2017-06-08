#include <ktest.h>
#include <common.h>
#include <dev/mc146818reg.h>
#include <dev/isareg.h>
#include <pci.h>
#include <interrupt.h>
#include <klog.h>
#include <sleepq.h>
#include <time.h>

#define RTC_ADDR (IO_RTC + 0)
#define RTC_DATA (IO_RTC + 1)

typedef struct rtc_state {
  resource_t *regs;
  intr_handler_t intr_handler;
} rtc_state_t;

static inline uint8_t rtc_read(resource_t *regs, unsigned addr) {
  bus_space_write_1(regs, RTC_ADDR, addr);
  return bus_space_read_1(regs, RTC_DATA);
}

static inline void rtc_write(resource_t *regs, unsigned addr, uint8_t value) {
  bus_space_write_1(regs, RTC_ADDR, addr);
  bus_space_write_1(regs, RTC_DATA, value);
}

static inline void rtc_setb(resource_t *regs, unsigned addr, uint8_t mask) {
  rtc_write(regs, addr, rtc_read(regs, addr) | mask);
}

static void rtc_gettime(resource_t *regs, tm_t *t) {
  t->tm_sec = rtc_read(regs, MC_SEC);
  t->tm_min = rtc_read(regs, MC_MIN);
  t->tm_hour = rtc_read(regs, MC_HOUR);
  t->tm_wday = rtc_read(regs, MC_DOW);
  t->tm_mday = rtc_read(regs, MC_DOM);
  t->tm_mon = rtc_read(regs, MC_MONTH);
  t->tm_year = rtc_read(regs, MC_YEAR) + 2000;
}

static intr_filter_t rtc_intr(void *data) {
  rtc_state_t *rtc = data;
  uint8_t regc = rtc_read(rtc->regs, MC_REGC);
  if (regc & MC_REGC_PF) {
    sleepq_signal(rtc_intr);
    return IF_FILTERED;
  }
  return IF_STRAY;
}

static int rtc_attach(device_t *dev) {
  assert(dev->parent->bus == DEV_BUS_PCI);

  pci_bus_state_t *pcib = dev->parent->state;
  rtc_state_t *rtc = dev->state;

  rtc->regs = pcib->io_space;

  rtc->intr_handler =
    INTR_HANDLER_INIT(rtc_intr, NULL, rtc, "RTC periodic timer", 0);
  bus_intr_setup(dev, 8, &rtc->intr_handler);

  /* Configure how the time is presented through registers. */
  rtc_setb(rtc->regs, MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);

  /* Set RTC timer so that it triggers interrupt 2 times per second. */
  rtc_write(rtc->regs, MC_REGA, MC_RATE_2_Hz);
  rtc_setb(rtc->regs, MC_REGB, MC_REGB_PIE);

  return 0;
}

static driver_t rtc_driver = {
  .desc = "MC146818 RTC driver mockup",
  .size = sizeof(rtc_state_t),
  .attach = rtc_attach,
};

extern device_t *gt_pci;

static int test_rtc(void) {
  device_t *rtcdev = make_device(gt_pci, &rtc_driver);
  rtc_state_t *rtc = rtcdev->state;

  while (1) {
    tm_t t;
    rtc_gettime(rtc->regs, &t);

    klog("Time is %02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);

    sleepq_wait(rtc_intr, "RTC 2Hz interrupt");
  }

  return KTEST_FAILURE;
}

KTEST_ADD(rtc, test_rtc, KTEST_FLAG_NORETURN);
