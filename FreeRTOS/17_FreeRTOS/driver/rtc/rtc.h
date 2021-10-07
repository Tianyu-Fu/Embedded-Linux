#ifndef __RTC_H
#define __RTC_H

#include "imx6u.h"

/* 时间有关的宏定义 */
#define SECONDS_IN_DAY (86400)
#define SECONDS_IN_HOUR (3600)
#define SECONDS_IN_MINUTE (60)
#define DAYS_IN_YEAR (365)
#define YEAR_START (1970)
#define YEAR_END (2099)

/* 时间有关的结构体 */
struct rtc_datetime{
    unsigned int year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
};

void rtc_init(void);
void rtc_enable(void);
void rtc_disable(void);
unsigned char rtc_isleapyear(unsigned short year);
unsigned int rtc_coverdate_to_seconds(struct rtc_datetime *datetime);
void rtc_setdatetime(struct rtc_datetime *datetime);
unsigned int rtc_getseconds(void);
void rtc_getdatetime(struct rtc_datetime *datetime);
void rtc_convertseconds_to_datetime(unsigned int seconds, struct rtc_datetime *datetime);

#endif