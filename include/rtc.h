#ifndef __RTC_H__
#define __RTC_H__

typedef struct {
  int sec;
  int min;
  int hour;
  int wday;
  int mday;
  int month;
  int year;
} rtc_time_t;

void rtc_init(void);
void rtc_read(rtc_time_t *t);

#endif
