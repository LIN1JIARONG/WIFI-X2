/*
 * iot_rtc.c
 *
 *  Created on: 2021年12月3日
 *      Author: Administrator
 */

#include "iot_rtc.h"
#include "iot_system.h"
#include "esp_sntp.h"
//设置时钟
//把输入的时钟转换为秒钟
//以1970年1月1日为基准
//1970~2099年为合法年份
//返回值:0,成功;其他:错误代码.
//月份数据表
 const uint8_t table_week[12]= {0,3,3,6,1,4,6,2,5,0,3,5}; //月修正数据表
//平年的月份日期表
const uint8_t mon_table[12]= {31,28,31,30,31,30,31,31,30,31,30,31};
uint8_t Clock_Time_Init[7]= {22,7,10,10,10,10,1}; //格式年 月 日 时 分 秒缓存数组


RTC_TimeTypeDef sTime = {0};

#if 0
//判断是否是闰年函数
//月份   1  2  3  4  5  6  7  8  9  10 11 12
//闰年   31 29 31 30 31 30 31 31 30 31 30 31
//非闰年 31 28 31 30 31 30 31 31 30 31 30 31
//输入:年份
//输出:该年份是不是闰年.1,是.0,不是
uint8_t Is_Leap_Year(uint16_t year)
{

    if(year%4==0) //必须能被4整除
    {
        if(year%100==0)
        {
            if(year%400==0)
            {
                return 1;    //如果以00结尾,还要能被400整除
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 1;
        }
    }

     return 0;
}

//获得现在是星期几
//功能描述:输入公历日期得到星期(只允许1901-2099年)
//输入参数：公历年月日
//返回值：星期号
uint8_t RTC_Get_Week(uint16_t year,uint8_t month,uint8_t day)
{
    uint16_t temp2;
    uint8_t yearH,yearL;

    yearH=year/100;
    yearL=year%100;
    // 如果为21世纪,年份数加100
    if(yearH>19)
    {
        yearL+=100;
    }
    // 所过闰年数只算1900年之后的
    temp2=yearL+yearL/4;
    temp2=temp2%7;
    temp2=temp2+day+table_week[month-1];
    if(yearL%4==0&&month<3)
    {
        temp2--;
    }
    return(temp2%7);
}
#endif

void IOT_CLOCK_SOFTInit(void)
{
	uint8_t sysClockdate[20]={0};

// 读取31寄存器
//	memcpy(&pbuf[CONFIG_LEN_BUFADDR],&Collector_Config_LenList,Parameter_len_MAX);	 //更新寄存器的长度链表
//	    NOWTime[0]='2';
//	    NOWTime[1]='0';
//	  	NOWTime[2]= sTime.data.Year /10 +0x30;
//	 	NOWTime[3]= sTime.data.Year %10 +0x30;
//		NOWTime[4]='-';
//		NOWTime[5]= sTime.data.Month /10 +0x30;
//		NOWTime[6]= sTime.data.Month %10 +0x30;
//		NOWTime[7]='-';
//		NOWTime[8]= sTime.data.Date /10 +0x30;
//		NOWTime[9]= sTime.data.Date %10 +0x30;
//		NOWTime[10]=' ';
//		NOWTime[11]= sTime.data.Hours /10 +0x30;
//		NOWTime[12]= sTime.data.Hours %10 +0x30;
//		NOWTime[13]= ':';
//		NOWTime[14]= sTime.data.Minutes /10 +0x30;
//		NOWTime[15]= sTime.data.Minutes %10 +0x30;
//		NOWTime[16]= ':';
//		NOWTime[17]= sTime.data.Seconds /10 +0x30;
//		NOWTime[18]= sTime.data.Seconds %10 +0x30;
//		memcpy(&SysTAB_Parameter[Parameter_len[31]],NOWTime,19);

    memcpy(sysClockdate,&SysTAB_Parameter[Parameter_len[31]],19);
    Clock_Time_Init[0]=(sysClockdate[2]-0x30)*10+(sysClockdate[3]-0x30);  //年
    Clock_Time_Init[1]=(sysClockdate[5]-0x30)*10+(sysClockdate[6]-0x30);  //月
    Clock_Time_Init[2]=(sysClockdate[8]-0x30)*10+(sysClockdate[9]-0x30);  //日
    Clock_Time_Init[3]=(sysClockdate[11]-0x30)*10+(sysClockdate[12]-0x30);  //时
    Clock_Time_Init[4]=(sysClockdate[14]-0x30)*10+(sysClockdate[15]-0x30);  //分
    Clock_Time_Init[5]=(sysClockdate[17]-0x30)*10+(sysClockdate[18]-0x30);  //秒

  sTime.Clock_Time_Tab[0]=Clock_Time_Init[0];
  sTime.Clock_Time_Tab[1]=Clock_Time_Init[1];
  sTime.Clock_Time_Tab[2]=Clock_Time_Init[2];
  sTime.Clock_Time_Tab[3]=Clock_Time_Init[3];
  sTime.Clock_Time_Tab[4]=Clock_Time_Init[4];
  sTime.Clock_Time_Tab[5]=Clock_Time_Init[5];

//  IOT_Printf("TIME1:20%d:%02d:%02d :%02d:%02d:%02d \r\n", sTime.data.Year,sTime.data.Month,sTime.data.Date,sTime.data.Hours,sTime.data.Minutes,sTime.data.Seconds);
////IOT_RTC_Set(sTime.Clock_Time_Tab[0],sTime.Clock_Time_Tab[1],sTime.Clock_Time_Tab[2],sTime.Clock_Time_Tab[3],sTime.Clock_Time_Tab[4],sTime.Clock_Time_Tab[5]);
  IOT_RTCSetTime(sTime.Clock_Time_Tab);

  IOT_Printf("IOT_CLOCK_SOFTInit \r\n");
  IOT_Printf("TIME2:20%d:%02d:%02d :%02d:%02d:%02d \r\n", sTime.data.Year,sTime.data.Month,sTime.data.Date,sTime.data.Hours,sTime.data.Minutes,sTime.data.Seconds);
  //IOT_RTC_Get();   //屏蔽  刚写就读取，有可能写入失败
  //IOT_ESP_RTCGetTime();

}
void IOT_printf_time(void)
{
	IOT_Printf("TIME:20%d:%02d:%02d :%02d:%02d:%02d \r\n", sTime.data.Year,sTime.data.Month,sTime.data.Date,sTime.data.Hours,sTime.data.Minutes,sTime.data.Seconds);
 // IOT_Printf("%lld\r\n",esp_timer_get_time());
}
#if 0
uint32_t IOT_GET_SOFT_sec(void)
{
	return System_state.SoftClockTick;
}
uint32_t IOT_SET_SOFT_sec(uint32_t setSEC )
{
	System_state.SoftClockTick =setSEC;
	return  0;
}

uint8_t IOT_RTC_Set(uint16_t syear,uint8_t smon,uint8_t sday,uint8_t hour,uint8_t min,uint8_t sec)
{

    uint16_t t;
    uint32_t seccount=0;
    syear=syear+2000;
    if(syear<1970||syear>2099)
    {
        return 1;
    }
    for(t=1970; t<syear; t++) //把所有年份的秒钟相加
    {
        if(Is_Leap_Year(t))
        {
            seccount+=31622400;    //闰年的秒钟数
        }
        else
        {
            seccount+=31536000;    //平年的秒钟数
        }
    }

    smon-=1;
    for(t=0; t<smon; t++)  //把前面月份的秒钟数相加
    {
        seccount+=(uint32_t)mon_table[t]*86400;//月份秒钟数相加
        if(Is_Leap_Year(syear)&&t==1)
        {
            seccount+=86400;    //闰年2月份增加一天的秒钟数
        }
    }

    seccount+=(uint32_t)(sday-1)*86400;//把前面日期的秒钟数相加

    seccount+=(uint32_t)hour*3600;//小时秒钟数

    seccount+=(uint32_t)min*60;   //分钟秒钟数

    seccount+=sec;//最后的秒钟加上去

    System_state.SoftClockTick=seccount ;

    IOT_SET_SOFT_sec(seccount);

    IOT_Printf(" System_state.SoftClockTick=%d \r\n",System_state.SoftClockTick);
    return 0;

}

uint8_t IOT_RTC_Get(void)
{

    uint32_t timecount=0;
    uint32_t temp=0;
    uint16_t temp1=0;

	timecount =IOT_GET_SOFT_sec();
//	IOT_Printf("timecount=%d \r\n",timecount);
    temp=timecount/86400;   //得到天数(秒钟数对应的)
    if(temp > 0)//超过一天了
    {
        temp1=1970; //从1970年开始
        while(temp>=365)
        {
            if(Is_Leap_Year(temp1))//是闰年
            {
                if(temp>=366)
                {
                    temp-=366;    //闰年的秒钟数
                }
                else
                {
                    break;
                }
            }
            else
            {
                temp-=365;    //平年
            }
            temp1++;
        }
        sTime.data.Year=temp1;//得到年份
        temp1=0;
        while(temp>=28)//超过了一个月
        {
            if(Is_Leap_Year(sTime.data.Year)&&temp1==1)//当年是不是闰年/2月份
            {
                if(temp>=29)
                {
                    temp-=29;    //闰年的秒钟数
                }
                else
                {
                    break;
                }
            }
            else
            {
                if(temp>=mon_table[temp1])
                {
                    temp-=mon_table[temp1];    //平年
                }
                else
                {
                    break;
                }
            }
            temp1++;
        }
        sTime.data.Month=temp1+1;//得到月份
        sTime.data.Date=temp+1;  //得到日期
    }
    temp=timecount%86400;     //得到秒钟数
    sTime.data.Hours=temp/3600;     //小时
    sTime.data.Minutes=(temp%3600)/60; //分钟
    sTime.data.Seconds=(temp%3600)%60; //秒钟
    sTime.data.WeekDay=RTC_Get_Week(sTime.data.Year,sTime.data.Month,sTime.data.Date);//获取星期
    sTime.data.Year=sTime.data.Year-2000;
//  IOT_Printf("sTime.data.Year=%d \r\n",sTime.data.Year);

    return 0;
}

#endif

uint8_t IOT_RTCSetTime(uint8_t *pTABtime)
{

	if((pTABtime[0] < 22) || (pTABtime[0] > 99) || (pTABtime[1] > 12) || (pTABtime[2] > 31)
			|| (pTABtime[3] > 24) || (pTABtime[4] > 59) || (pTABtime[5] > 59))
	{
		IOT_Printf("\r\n******IOT_RTCSetTime**ERR0R* %02d-%02d-%02d %02d:%02d:%02d*****************\r\n" ,pTABtime[0],pTABtime[1],pTABtime[2],pTABtime[3],pTABtime[4],pTABtime[5]);

		return 1; // 失败
	}
	IOT_Printf("\r\n******IOT_RTCSetTime*** %02d-%02d-%02d %02d:%02d:%02d*****************\r\n" ,pTABtime[0],pTABtime[1],pTABtime[2],pTABtime[3],pTABtime[4],pTABtime[5]);

	//IOT_RTC_Set(pTABtime[0],pTABtime[1],pTABtime[2],pTABtime[3],pTABtime[4],pTABtime[5]);
	IOT_ESP_RTCSet(pTABtime);
	return 0;

}

/*******************************************************************************
 * FunctionName : IOT_ESP_RTCSet
 * Description  : 设置ESP 的RTC时间
 * Parameters   : uint8_t *pTABtime    要设置的时间
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_ESP_RTCSet(uint8_t *pTABtime)
{
	time_t unix_time = 0;
	struct  tm Stm = {0};
	//sntp_restart();
	sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);		//设置平滑更新时间模式,可减少误差

	Stm.tm_year = pTABtime[0] + 100;  //年 从1900年开始
	Stm.tm_mon	= pTABtime[1] - 1;		  //月	0 ~ 11
	Stm.tm_mday	= pTABtime[2];		  //日	1 ~ 31
	Stm.tm_hour = pTABtime[3];		  //时	0 ~ 23
	Stm.tm_min  = pTABtime[4];		  //分	0 ~ 59
	Stm.tm_sec =  pTABtime[5];		  //秒	0 ~ 59


	unix_time = mktime(&Stm);	//转换为unix时间戳 , 该时间为+8区时间

 // unix_time -= 8 * 60 * 60; //转换为 中时区(零时区) 时间
//	setenv("TZ", "CST-8", 1);	//设置时间为+8区时间
//	tzset();

	struct timeval tv ={
			.tv_sec =  unix_time,	//秒 	s
			.tv_usec = 0,			//微秒	us
 	};

	sntp_sync_time(&tv);			//设置RTC时间

}


/*******************************************************************************
 * FunctionName : IOT_ESP_RTCGet_Unix
 * Description  : 获取ESP 的RTC unix 时间
 * Parameters   : void
 * Returns      : time_t uinx时间戳
 * Notice       : 时间戳时间为 零时区
*******************************************************************************/
time_t IOT_ESP_RTCGet_Unix(void)
{
	time_t unix_time = 0;


	time(&unix_time);

	return unix_time;
}

/*******************************************************************************
 * FunctionName : IOT_ESP_RTCGetTime
 * Description  : 获取ESP 的RTC时间
 * Parameters   : void
 * Returns      : none
 * Notice       : none
*******************************************************************************/
time_t IOT_ESP_RTCGetTime(void)
{
	time_t unix_time = 0;
	struct tm timeinfo = {0};

//	setenv("TZ", "CST-8", 1);	//设置时间为+8区时间
//	tzset();

	unix_time = IOT_ESP_RTCGet_Unix();	//获取unix时间戳

	localtime_r(&unix_time, &timeinfo);  //获取当地时间(+8区)

	sTime.data.Year = timeinfo.tm_year - 100;
	sTime.data.Month = timeinfo.tm_mon + 1;
	sTime.data.Date = timeinfo.tm_mday;
	sTime.data.Hours = timeinfo.tm_hour;
	sTime.data.Minutes = timeinfo.tm_min;
	sTime.data.Seconds = timeinfo.tm_sec;

	return unix_time;
}




