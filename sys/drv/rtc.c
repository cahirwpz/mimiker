/* MC146818 Real-time clock driver */
#include <sys/mimiker.h>
#include <dev/mc146818reg.h>
#include <dev/isareg.h>
#include <sys/bus.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/libkern.h>
#include <sys/devfs.h>
#include <sys/sleepq.h>
#include <sys/time.h>
#include <sys/timer.h>
#include <sys/vnode.h>
#include <sys/devclass.h>

#define RTC_ADDR 0
#define RTC_DATA 1

#define RTC_ASCTIME_SIZE 32

typedef struct rtc_state {
  resource_t *regs;
  char asctime[RTC_ASCTIME_SIZE];
  unsigned counter; /* TODO Should that be part of intr_handler_t ? */
  resource_t *irq_res;
} rtc_state_t;

/*
 * TODO This way of handling MC146818 RTC device is specific to ISA bus.
 * Registers should be accessible with single res_{read,write}_1
 * operations on 16 bytes wide bus.
 *
 * Hopefully this design issue will get resolved after more work is put into
 * resource management and ISA bus driver.
 */

static void boottime_init(tm_t *t) {
  bintime_t bt = BINTIME(tm2sec(t));
  tm_setclock(&bt);
}

static inline uint8_t rtc_read(resource_t *regs, unsigned addr) {
  bus_write_1(regs, RTC_ADDR, addr);
  return bus_read_1(regs, RTC_DATA);
}

static inline void rtc_write(resource_t *regs, unsigned addr, uint8_t value) {
  bus_write_1(regs, RTC_ADDR, addr);
  bus_write_1(regs, RTC_DATA, value);
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
  t->tm_mon = rtc_read(regs, MC_MONTH) - 1;
  t->tm_year = rtc_read(regs, MC_YEAR) + 100;
}

static intr_filter_t rtc_intr(void *data) {
  rtc_state_t *rtc = data;
  uint8_t regc = rtc_read(rtc->regs, MC_REGC);
  if (regc & MC_REGC_PF) {
    if (rtc->counter++ & 1)
      sleepq_signal(rtc);
    return IF_FILTERED;
  }
  return IF_STRAY;
}

static int rtc_time_read(vnode_t *v, uio_t *uio, int ioflag) {
  rtc_state_t *rtc = devfs_node_data(v);
  tm_t t;

  uio->uio_offset = 0; /* This device does not support offsets. */
  sleepq_wait(rtc, NULL);
  rtc_gettime(rtc->regs, &t);
  int count = snprintf(rtc->asctime, RTC_ASCTIME_SIZE, "%d %d %d %d %d %d",
                       t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                       t.tm_min, t.tm_sec);
  if (count >= RTC_ASCTIME_SIZE)
    return EINVAL;
  return uiomove_frombuf(rtc->asctime, count, uio);
}

static vnodeops_t rtc_time_vnodeops = {.v_read = rtc_time_read};

static int rtc_attach(device_t *dev) {
  rtc_state_t *rtc = dev->state;

  rtc->regs = device_take_ioports(dev, 0, RF_ACTIVE);
  assert(rtc->regs != NULL);

  rtc->irq_res = device_take_irq(dev, 0, RF_ACTIVE);
  bus_intr_setup(dev, rtc->irq_res, rtc_intr, NULL, rtc, "RTC periodic timer");

  /* Configure how the time is presented through registers. */
  rtc_setb(rtc->regs, MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);

  /* Set RTC timer so that it triggers interrupt 2 times per second. */
  rtc_write(rtc->regs, MC_REGA, MC_RATE_2_Hz);
  rtc_setb(rtc->regs, MC_REGB, MC_REGB_PIE);

  /* Prepare /dev/rtc interface. */
  devfs_makedev(NULL, "rtc", &rtc_time_vnodeops, rtc, NULL);

  tm_t t;

  rtc_gettime(rtc->regs, &t);
  boottime_init(&t);

  return 0;
}

static int rtc_probe(device_t *dev) {
  return dev->unit == 2; /* XXX: unit 2 assigned by gt_pci */
}

/* clang-format off */
static driver_t rtc_driver = {
  .desc = "MC146818 RTC driver",
  .size = sizeof(rtc_state_t),
  .attach = rtc_attach,
  .probe = rtc_probe
};
/* clang-format on */

DEVCLASS_ENTRY(isa, rtc_driver);
