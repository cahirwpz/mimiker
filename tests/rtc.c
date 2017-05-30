#include <common.h>
#include <mips/malta.h>
#include <dev/mc146818reg.h>
#include <ktest.h>
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

static void tv_delay(timeval_t delay) {
  timeval_t now = get_uptime();
  timeval_t end = timeval_add(&now, &delay);
  while (timeval_cmp(&now, &end, <))
    now = get_uptime();
}

static int test_rtc() {
  rtc_setb(MC_REGB, MC_REGB_BINARY | MC_REGB_24HR | MC_REGB_PIE);

  while (1) {
    tm_t t;
    rtc_gettime(&t);

    uint8_t regc = rtc_read(MC_REGC);

    kprintf("Time is %02d:%02d:%02d, C(%02x)\n", t.tm_hour, t.tm_min, t.tm_sec,
            regc);

    tv_delay(TIMEVAL(1.0));
  }
  return KTEST_FAILURE;
}

KTEST_ADD(rtc, test_rtc, KTEST_FLAG_NORETURN);
