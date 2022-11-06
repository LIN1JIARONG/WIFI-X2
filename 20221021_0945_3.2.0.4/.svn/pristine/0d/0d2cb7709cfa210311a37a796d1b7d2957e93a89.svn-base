/*
 * iot_timer.c
 *
 *  Created on: 2021年11月18日
 *      Author: Administrator
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/timer.h"
#include "iot_rtc.h"
#include "iot_system.h"
#include "iot_mqtt.h"
#include "esp_log.h"
#include "time.h"
#include "freertos/semphr.h"

#include "iot_led.h"
#include "iot_InvWave.h"
#define TIMER_DIVIDER         (16) 		 //  Hardware timer clock divider 硬件定时器时钟分频器
#define TIMER_SCALE_SEC       (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds 将计数器值转换为秒
								  //80*1000000 /16    计算频率
#define TIMER_SCALE_MS        (TIMER_SCALE_SEC / 1000)  //  将计数器值转换为毫秒


static const char *TIMER_log = "TIMER_log";

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} example_timer_info_t;

static SemaphoreHandle_t Time_Semaphore = NULL;
//static xQueueHandle s_timer_queue;    //队列句柄
/**EventGroupHandle_t
 * @brief A sample structure to pass events from the timer ISR to task
**
***/
//typedef struct {
//    example_timer_info_t info;
//    uint64_t timer_counter_value;
//} example_timer_event_t;
/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */

//static void inline print_timer_counter(uint64_t counter_value)
//{
//     ESP_LOGI(TIMER_log,"Counter: 0x%08x%08x\r\n", (uint32_t) (counter_value >> 32), (uint32_t) (counter_value));
//     ESP_LOGI(TIMER_log,"Time   : %.8f s\r\n", (double) counter_value / TIMER_SCALE);
//}

static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
	static BaseType_t xHigherPriorityTaskWoken;	//标记退出此函数以后是否进行任务切换，这个变量的值由函数设置,用户无需关心
	//uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(TIMER_GROUP_0,TIMER_0);
	example_timer_info_t *info = (example_timer_info_t *) args;

	if((info != NULL) && (info->timer_group == TIMER_GROUP_0))
	 xSemaphoreGiveFromISR( Time_Semaphore, &xHigherPriorityTaskWoken );	//释放二值信号量

	return 0;

}

/**
 * @brief Initialize selected timer of timer group
 *
 * @param group Timer Group number, index from 0
 * @param timer timer ID, index from 0
 * @param auto_reload whether auto-reload on alarm event
 * @param timer_interval_sec interval of alarm
 */
static void example_tg_timer_init(int group, int timer, bool auto_reload, int timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload,
    }; // default clock source is APB
    timer_init(group, timer, &config);  //初始化和配置定时器。

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, timer, 0);  //设置计数器值为硬件定时器。

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE_MS ); //设置定时器报警值。 单位ms
    timer_enable_intr(group, timer);  //启用定时器中断。

    example_timer_info_t *timer_info = calloc(1, sizeof(example_timer_info_t));
	timer_info->timer_group = group;
	timer_info->timer_idx = timer;
	timer_info->auto_reload = auto_reload;
	timer_info->alarm_interval = timer_interval_sec;

    timer_isr_callback_add(group, timer, timer_group_isr_callback , timer_info, 0);  //添加中段回调函数
    timer_start(group, timer);  //启动硬件定时器计数器
}



void IOT_timer_Init(void )
{

	 vSemaphoreCreateBinary( Time_Semaphore );		//创建二值信号量
	 ESP_LOGI(TIMER_log,"IOT_timer_Initing\n");
     example_tg_timer_init(TIMER_GROUP_0, TIMER_0, true,200);	//周期设置200ms
	 ESP_LOGI(TIMER_log,"IOT_timer_Init OK\r\n");

     return;
}

uint8_t timetestdate[8]={22,5,27,18,53,1,1,0};

void IOT_Timer_task(void *arg)
{
//	esp_log_level_set(TIMER_log, ESP_LOG_INFO);

    ESP_LOGI(TIMER_log,"IOT_Timer_task \n");
    uint8_t Delaytime_MSnum = 0;
    uint8_t Delaytime_Snum = 0;
//    long long Rec_time = 0;
//    long long New_time = 0;
#if DEBUG_HEAPSIZE
	static	uint8_t timer_logof=0;
#endif

    ESP_LOGI(TIMER_log,"IOT_Timer_task \n");
    while(1)
    {
		if( xSemaphoreTake( Time_Semaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
		{
			InverterWaveWaitTime_Count();
			if(++Delaytime_MSnum >= 5)
			{
				Delaytime_MSnum= 0;
 				//System_state.SoftClockTick++ ;
				IOT_TCPDelaytimeSet_uc(ESP_WRITE,0);
			//	New_time = esp_timer_get_time();
			//	IOT_Printf("New_time - Rec_time = %lld\r\n",New_time - Rec_time);
			//	IOT_Printf("New_time = %lld\r\n",New_time);
			//	Rec_time = New_time;
			//	IOT_Printf("* System_state.SoftClockTick = %d*\r\n",System_state.SoftClockTick);

			//	ESP_LOGI(TIMER_log, "IOT_Timer_task TIME:20%d:%02d:%02d :%02d:%02d:%02d \r\n", sTime.data.Year,sTime.data.Month,sTime.data.Date,sTime.data.Hours,sTime.data.Minutes,sTime.data.Seconds);


				if(++Delaytime_Snum>5)
				{
					Delaytime_Snum=0;
					IOT_ESP_RTCGetTime();
					IOT_printf_time();
				}

            //  ESP_LOGI(TIMER_log, " ************Mqtt_state.MqttDATA_Time=%d** \r\n",Mqtt_state.MqttDATA_Time);
		    	//移动到 time定时器任务
				#if test_wifi
					IOT_LED_Polling();
				#else
					IOT_LED_Task();  // delay
				#endif

			}
			//xSemaphoreGive( Time_Semaphore );	//释放二值信号量
		}


#if DEBUG_HEAPSIZE
        if( timer_logof ++ >50)
        {
        	timer_logof =0;
        	ESP_LOGI(TIMER_log, "is IOT_Timer_task uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
        }
#endif
//        vTaskDelay(1000 / portTICK_PERIOD_MS);// 处理时间不能大于 队列事件的时间  正常不需要这个delay

    }
}


long long IOT_GetTick(void)
{
	return (esp_timer_get_time() / 1000);
}


