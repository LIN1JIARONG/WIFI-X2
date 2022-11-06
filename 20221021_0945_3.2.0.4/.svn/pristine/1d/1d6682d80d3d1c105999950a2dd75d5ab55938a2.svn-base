/*
 * iot_rtc.h
 *
 *  Created on: 2021年12月3日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_RTC_H_
#define COMPONENTS_INCLUDES_IOT_RTC_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef union
{
    struct
    {
        uint8_t Year ;
        uint8_t Month;
        uint8_t Date ;
        uint8_t Hours ;
        uint8_t Minutes ;
        uint8_t Seconds;
        uint8_t WeekDay ;
        uint8_t redate;
    } data;
    uint8_t Clock_Time_Tab[8];//格式年 月 日 时 分 秒缓存数组

} RTC_TimeTypeDef;

extern RTC_TimeTypeDef sTime;

extern uint8_t Clock_Time_Init[7] ; //格式年 月 日 时 分 秒缓存数组

void IOT_CLOCK_SOFTInit(void);
void IOT_printf_time(void);

#if 0
uint32_t IOT_GET_SOFT_sec(void);
uint32_t IOT_SET_SOFT_sec(uint32_t setSEC );
uint8_t IOT_RTC_Set(uint16_t syear,uint8_t smon,uint8_t sday,uint8_t hour,uint8_t min,uint8_t sec);

uint8_t IOT_RTC_Get(void);
#endif

uint8_t IOT_RTCSetTime(uint8_t *pTABtime);


void IOT_ESP_RTCSet(uint8_t *pTABtime);
time_t IOT_ESP_RTCGetTime(void);
#endif /* COMPONENTS_INCLUDES_IOT_RTC_H_ */
