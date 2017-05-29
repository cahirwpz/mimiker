#include <common.h>
#include <mips/malta.h>
#include <rtc.h>
#include <ktest.h>

/* http://geezer.osdevbrasil.net/temp/rtc.txt */

#define RTC_ADDR_R *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(MALTA_RTC_ADDR))
#define RTC_DATA_R *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(MALTA_RTC_DATA))

#define REG_B 0x0B
#define REG_B_SET 128
#define REG_B_PIE 64
#define REG_B_AIE 32
#define REG_B_UIE 16
#define REG_B_DM 4
#define REG_B_24 2
#define REG_B_DSE 1

#define REG_C 0x0C
#define REG_C_IRQF 128
#define REG_C_PF 64
#define REG_C_AF 32
#define REG_C_UF 16

void rtc_init() {
  RTC_ADDR_R = REG_B;
  RTC_DATA_R = RTC_DATA_R | REG_B_DM | REG_B_24 | REG_B_PIE;
}

void rtc_read(rtc_time_t *t) {
  RTC_ADDR_R = 0;
  t->sec = RTC_DATA_R;
  RTC_ADDR_R = 2;
  t->min = RTC_DATA_R;
  RTC_ADDR_R = 4;
  t->hour = RTC_DATA_R;
  RTC_ADDR_R = 6;
  t->wday = RTC_DATA_R;
  RTC_ADDR_R = 7;
  t->mday = RTC_DATA_R;
  RTC_ADDR_R = 8;
  t->month = RTC_DATA_R;
  RTC_ADDR_R = 9;
  t->year = RTC_DATA_R + 2000;
}

#ifdef _KERNELSPACE
#include <stdc.h>
#include <clock.h>

static void tv_delay(timeval_t delay) {
  timeval_t now = get_uptime();
  timeval_t end = timeval_add(&now, &delay);
  while (timeval_cmp(&now, &end, <))
    now = get_uptime();
}

static int test_rtc() {
  rtc_init();

  while (1) {
    rtc_time_t rtc;
    rtc_read(&rtc);

    RTC_ADDR_R = REG_C;
    unsigned reg_c = RTC_DATA_R;

    kprintf("Time is %02d:%02d:%02d, C(%02x)\n", rtc.hour, rtc.min, rtc.sec,
            reg_c);

    tv_delay(TIMEVAL(1.0));
  }
  return KTEST_FAILURE;
}

KTEST_ADD(rtc, test_rtc, KTEST_FLAG_NORETURN);

#endif
