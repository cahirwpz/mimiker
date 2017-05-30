#include <ktest.h>
#include <common.h>
#include <mips/malta.h>
#include <dev/mc146818reg.h>
#include <pci.h>
#include <interrupt.h>
#include <klog.h>
#include <sleepq.h>
#include <time.h>

#define RTC_ADDR_R *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(MALTA_RTC_ADDR))
#define RTC_DATA_R *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(MALTA_RTC_DATA))

static inline uint8_t rtc_read(unsigned reg) {
  RTC_ADDR_R = reg;
  return RTC_DATA_R;
}

static inline void rtc_write(unsigned reg, uint8_t value) {
  RTC_ADDR_R = reg;
  RTC_DATA_R = value;
}

static inline void rtc_setb(unsigned reg, uint8_t mask) {
  rtc_write(reg, rtc_read(reg) | mask);
}

static void rtc_gettime(tm_t *t) {
  t->tm_sec = rtc_read(MC_SEC);
  t->tm_min = rtc_read(MC_MIN);
  t->tm_hour = rtc_read(MC_HOUR);
  t->tm_wday = rtc_read(MC_DOW);
  t->tm_mday = rtc_read(MC_DOM);
  t->tm_mon = rtc_read(MC_MONTH);
  t->tm_year = rtc_read(MC_YEAR) + 2000;
}

static intr_filter_t rtc_intr(void *arg) {
  uint8_t regc = rtc_read(MC_REGC);
  if (regc & MC_REGC_PF) {
    sleepq_signal(rtc_intr);
    return IF_FILTERED;
  }
  return IF_STRAY;
}

static INTR_HANDLER_DEFINE(rtc_intr_handler, rtc_intr, NULL, NULL, "RTC timer",
                           0);

extern device_t *gt_pci;

static int test_rtc() {
  pci_bus_driver_t *gt_pci_drv = (pci_bus_driver_t *)gt_pci->driver;
  gt_pci_drv->bus.intr_setup(gt_pci, 8, rtc_intr_handler);

  /* Configure how the time is presented through registers. */
  rtc_setb(MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);

  /* Set RTC timer so that it triggers interrupt 2 times per second. */
  rtc_write(MC_REGA, MC_RATE_2_Hz);
  rtc_setb(MC_REGB, MC_REGB_PIE);

  while (1) {
    tm_t t;
    rtc_gettime(&t);

    klog("Time is %02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);

    sleepq_wait(rtc_intr, "RTC 2Hz interrupt");
  }

  return KTEST_FAILURE;
}

KTEST_ADD(rtc, test_rtc, KTEST_FLAG_NORETURN);
