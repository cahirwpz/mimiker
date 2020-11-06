/* MC146818 Real-time clock driver */
#include <sys/mimiker.h>
#include <dev/mc146818reg.h>
#include <dev/isareg.h>
#include <sys/pci.h>
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
  intr_handler_t intr_handler;
  timer_t timer;
} rtc_state_t;

/*
 * TODO This way of handling MC146818 RTC device is specific to ISA bus.
 * Registers should be accessible with single res_{read,write}_1
 * operations on 16 bytes wide bus.
 *
 * Hopefully this design issue will get resolved after more work is put into
 * resource management and ISA bus driver.
 */

uint32_t estimate_freq(timer_t *tm, uint32_t freq);

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
  rtc_state_t *rtc = v->v_data;
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

static vnodeops_t rtc_time_vnodeops = {.v_open = vnode_open_generic,
                                       .v_read = rtc_time_read};

static int rtc_attach(device_t *dev) {
  assert(dev->parent->bus == DEV_BUS_PCI);

  vnodeops_init(&rtc_time_vnodeops);

  rtc_state_t *rtc = dev->state;

  rtc->regs = bus_alloc_resource(
    dev, RT_ISA, 0, IO_RTC, IO_RTC + IO_RTCSIZE - 1, IO_RTCSIZE, RF_ACTIVE);
  assert(rtc->regs != NULL);

  rtc->intr_handler =
    INTR_HANDLER_INIT(rtc_intr, NULL, rtc, "RTC periodic timer", 0);
  bus_intr_setup(dev, 8, &rtc->intr_handler);

  /* Configure how the time is presented through registers. */
  rtc_setb(rtc->regs, MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);

  /* Set RTC timer so that it triggers interrupt 2 times per second. */
  rtc_write(rtc->regs, MC_REGA, MC_RATE_2_Hz);
  rtc_setb(rtc->regs, MC_REGB, MC_REGB_PIE);

  /* Prepare /dev/rtc interface. */
  devfs_makedev(NULL, "rtc", &rtc_time_vnodeops, rtc);

  tm_t t;
  rtc_gettime(rtc->regs, &t);
  boottime_init(&t);

  rtc->timer = (timer_t){
    .tm_name = "MC146818 RTC",
    .tm_flags = TMF_STABLE,
    .tm_getfreq = estimate_freq,
    .tm_priv = dev,
  };

  tm_register(&rtc->timer);

  return 0;
}

static driver_t rtc_driver = {
  .desc = "MC146818 RTC driver",
  .size = sizeof(rtc_state_t),
  .attach = rtc_attach,
  .identify = bus_generic_identify,
};

DEVCLASS_ENTRY(pci, rtc_driver);

uint32_t estimate_freq(timer_t *tm, uint32_t freq) {
  device_t *dev = tm->tm_priv;
  rtc_state_t *rtc = dev->state;
  resource_t *regs = rtc->regs;

  rtc_setb(rtc->regs, MC_REGB, MC_REGB_BINARY);

  uint32_t secs1, secs2;
  bintime_t end = {0, 0}, start = {0, 0}, res;

  while ((rtc_read(regs, MC_REGA) & MC_REGA_UIP) == 0)
    ;
  while ((rtc_read(regs, MC_REGA) & MC_REGA_UIP) != 0)
    ;
  start = binuptime();

  while ((rtc_read(regs, MC_REGA) & MC_REGA_UIP) == 0)
    ;
  secs1 = rtc_read(regs, MC_SEC);

  while ((rtc_read(regs, MC_REGA) & MC_REGA_UIP) != 0)
    ;
  end = binuptime();

  while (rtc_read(regs, MC_REGA) & MC_REGA_UIP)
    ;
  secs2 = rtc_read(regs, MC_SEC);

  assert(bintime_cmp(&end, &start, >));
  bintime_sub(&end, &start);
  res = bintime_mul(end, freq);

  assert(secs2 > secs1);
  return 10 * res.sec / (secs2 - secs1);
}