#ifndef __RTC_H__
#define __RTC_H__

void rtc_init(void);
void rtc_alarm_a_init(void);
void rtc_alarm_set(RTC_AlarmTypeDef RTC_AlarmStructure);
uint8_t DEC2BCD(uint8_t dec);


#endif

