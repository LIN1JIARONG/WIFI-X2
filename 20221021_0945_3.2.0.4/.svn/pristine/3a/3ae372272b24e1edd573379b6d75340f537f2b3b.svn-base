/*
 * iot_led.c
 *
 *  Created on: 2021年11月16日
 *      Author: Administrator
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "iot_led.h"
#include "esp_log.h"
#include "esp_system.h"
#include "iot_gatts_table.h"
#include "iot_protocol.h"
#include "iot_station.h"

#include "iot_station.h"
#include "esp_wifi.h"

#define Queue_Enable      0    //按键使用队列
#define Semaphore_Enable  1    //按键使用二值信号量


static const char *KEY = "KEY";

void IOT_GPIO_Init(void )
{
	gpio_reset_pin(LED1_PIN);
	gpio_set_direction(LED1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED1_PIN, 1);

	gpio_reset_pin(LED2_PIN);
	gpio_set_direction(LED2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED2_PIN, 1);


#if test_wifi
	gpio_reset_pin(LED3_PIN);
	gpio_set_direction(LED3_PIN, GPIO_MODE_OUTPUT);

	gpio_set_level(LEDBLUE_PIN,0); //灭
	gpio_set_level(LEDGREE_PIN,0); // 亮
	gpio_set_level(LEDREG_PIN,0); // 亮

#endif

}
void IOT_LED1_ON(void)
{
	 gpio_set_level(LED1_PIN, 1);
}
void IOT_LED1_OFF(void)
{
	 gpio_set_level(LED1_PIN, 0);
}
void IOT_LED2_ON(void)
{
	 gpio_set_level(LED2_PIN, 1);
}
void IOT_LED2_OFF(void)
{
	 gpio_set_level(LED2_PIN, 0);
}
void IOT_LED3_ON(void)
{
	 gpio_set_level(LED3_PIN, 1);
}
void IOT_LED3_OFF(void)
{
	 gpio_set_level(LED3_PIN, 0);
}


//1、三个灯都不亮，表示没有检测到光伏设备
//2、绿灯闪烁、连接不上服务器
//3、蓝灯闪烁、工作正常
//4、红灯常亮    wifi模块错误  读取flash错误      LEDREG_PIN
//5、绿灯常亮	 无法连接到路由器  				 LEDGREE_PIN
//6、蓝灯常亮    BLE 开启     				 LEDBLUE_PIN
//7、L  亮   H  灭

void IOT_LED_Polling(void)
{
	static uint8_t led_blinkflag=0;

	if( System_state.SYS_RestartFlag==1)
	{
		gpio_set_level(LEDBLUE_PIN,0); //常亮
		gpio_set_level(LEDGREE_PIN,0); //常亮
		gpio_set_level(LEDREG_PIN,0); //常亮
	}
	else if(System_state.ble_onoff == 1)		//蓝牙开启 蓝灯常亮
	{
		gpio_set_level(LEDBLUE_PIN,0); //常亮
		gpio_set_level(LEDGREE_PIN,1); //灭
		gpio_set_level(LEDREG_PIN,1); //灭
	}
	else if(System_state.Pv_state == 0 )  //没有连接上光伏设备	 全灭
	{
		 gpio_set_level(LEDGREE_PIN,1); //灭
		 gpio_set_level(LEDREG_PIN,1); //灭
		 gpio_set_level(LEDBLUE_PIN,1); //灭
	}
	else if(System_state.SmartConfig_OnOff_Flag == 1 )	 //20220727 chen 采集器未配网 红灯闪烁
	{
		led_blinkflag++;
		gpio_set_level(LEDGREE_PIN,1); //灭
		gpio_set_level(LEDBLUE_PIN,1); //灭
		gpio_set_level(LEDREG_PIN,0); //闪-亮
	}
	else if(System_state.Wifi_state == 0 )		//未连接路由器	//20220727 chen 绿灯常亮优先级提前
	{
		gpio_set_level(LEDGREE_PIN,0); //常亮
		gpio_set_level(LEDBLUE_PIN,1); //灭
		gpio_set_level(LEDREG_PIN,1); //灭
	}
	else if( System_state.Mbedtls_state==0 )
	{
		led_blinkflag++;
		gpio_set_level(LEDGREE_PIN,0); //闪-亮
		gpio_set_level(LEDREG_PIN,1);  //灭
		gpio_set_level(LEDBLUE_PIN,1); //灭
	}
	else if(System_state.Wifi_state == 1 && System_state.Pv_state == 1 && System_state.Mbedtls_state==1 )
	{
		led_blinkflag++;
		gpio_set_level(LEDBLUE_PIN,0); //闪-亮
		gpio_set_level(LEDREG_PIN,1);  //灭
		gpio_set_level(LEDGREE_PIN,1); //灭
	}


	if(led_blinkflag>=2)
	{
		led_blinkflag=0;
		//vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(LEDGREE_PIN,1); //灭
		gpio_set_level(LEDREG_PIN,1); //灭
		gpio_set_level(LEDBLUE_PIN,1); //灭
	}
//	vTaskDelay(1000 / portTICK_PERIOD_MS);

}

void IOT_LED_Task(void)
{
	static uint8_t bleled_blinkflag=0;
	static uint8_t wifiled_blinkflag=0;

	if(System_state.ble_onoff == 0)
	{
		IOT_LED1_OFF();
	}
	else if((System_state.ble_onoff == 1) &&(System_state.Ble_state == 0 ))
	{
		bleled_blinkflag++;
		IOT_LED1_ON();
	}
	else if(System_state.Ble_state == 1)
	{
		IOT_LED1_ON();
	}
	if(System_state.wifi_onoff == 0)
	{
		IOT_LED2_OFF();
	}
	else if((System_state.wifi_onoff == 1) && (System_state.Wifi_state ==  0 ))
	{
		wifiled_blinkflag++;
		IOT_LED2_ON();
	}
	else if(System_state.Wifi_state == 1)
	{
		IOT_LED2_ON();
	}
	if(bleled_blinkflag>=2)  //闪烁
	{
		bleled_blinkflag=0;
		IOT_LED1_OFF(); //灭
	}
	if(wifiled_blinkflag>=2)
	{
		wifiled_blinkflag=0;
		IOT_LED2_OFF(); //灭
	}

}


//队列方式：多个按键情况，写入队列，再读取处理
// 定义按下按钮枚举类型
typedef enum {
	KEY_SHORT_PRESS = 1,
	KEY_LONG_PRESS,
} alink_key_t;

QueueHandle_t  keyevt_queue =NULL;

//二值信号量可以在某个特殊的中断发送时，让任务解除阻塞，相当于让任务与中断同步。
//可以让中断事件处理量大的工作在同步任务中完成，中断服务例程(ISR)中只是快速处理少部分工作。
//二值信号量常应用于同步
#if Semaphore_Enable

#define MIN_DELAY 200
static xSemaphoreHandle Key_handle = NULL;

#endif

//按钮中断处理
void IRAM_ATTR Key_isr_handler(void* arg)
{
#if Semaphore_Enable
  int last_time = 0;
// 获取系统当前运行的时钟节拍数, 此函数用于在中断服务程序里面调用, 如果在任务里面调用的话，
// 需要使用函数 xTaskGetTickCount, 这两个函数切不可混用
  int now = xTaskGetTickCountFromISR();
  if (now - last_time > MIN_DELAY)
  {
     int ignored = pdTRUE;
     //通过释放信号量来使任务解除阻塞
     xSemaphoreGiveFromISR(Key_handle, &ignored);
     last_time = now;
  }
#endif
#if Queue_Enable
  uint32_t gpio_num = (uint32_t) arg;
  xQueueSendFromISR(keyevt_queue, &gpio_num, NULL);  // 队列可以用于多个按键 把按键写入队列
#endif
}

void Key_task(void* arg)
{
  uint32_t io_num;
  BaseType_t press_key   = pdFALSE;
  BaseType_t release_key = pdFALSE;
  int backup_time = 0;

  uint8_t key_SHORT=0;
  uint8_t key_MEDIUM=0;
  uint8_t key_LONG=0;

#if DEBUG_HEAPSIZE
	static	uint8_t timer_logoi=0;
#endif

  ESP_LOGI(KEY, "is Key_task uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));

  while (1)
  {
#if Semaphore_Enable
	  //获取信号量，portMAX_DELAY标识一直等待，可以设置具体的时钟滴答次数
	    if( xSemaphoreTake( Key_handle,  3500/ portTICK_PERIOD_MS) == pdTRUE )
	 	{
//	      ESP_LOGI(KEY, "is Key_task uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
			if (gpio_get_level(KEY_PIN) == 0) {//当前低电平，记录下用户按下按键的时间点
				press_key = pdTRUE;
				backup_time = esp_timer_get_time();
				//如果当前GPIO口的电平已经记录为按下，则开始减去上次按下按键的时间点
			} else if (press_key) {
				//记录抬升时间点
				release_key = pdTRUE;
				backup_time = esp_timer_get_time() - backup_time;
			}
			else	//超时按键
			{

			}
	 	}
 	    else  //按键超时不送开
	    {
 	    	if(press_key == pdTRUE)
 	    	{
 	    		 release_key = pdTRUE;
 	    		 backup_time = esp_timer_get_time() - backup_time;
 	    	}
	    //	 ESP_LOGI(KEY, "is Key_task   8s  pdTRUE  press_key=%d, release_key =%d!!!\r\n",press_key,release_key);
	    }


#endif
#if Queue_Enable
	    // 接收从消息队列发来的消息
		xQueueReceive(keyevt_queue, &io_num, portMAX_DELAY);
		ESP_LOGI(KEY, "is Key_task keyevt_queue= %d !!!\r\n", io_num);
		if (gpio_get_level(io_num) == 0) {//当前低电平，记录下用户按下按键的时间点
			press_key = pdTRUE;
			backup_time = esp_timer_get_time();
			//如果当前GPIO口的电平已经记录为按下，则开始减去上次按下按键的时间点
		} else if (press_key) {
			//记录抬升时间点
			release_key = pdTRUE;
			backup_time = esp_timer_get_time() - backup_time;
		}
#endif
		//近当按下标志位和按键弹起标志位都为1时候，才执行回调
		if (press_key & release_key)
		{
			press_key = pdFALSE;
			release_key = pdFALSE;
			//如果大于1s则回调长按，否则就短按回调
			if (backup_time > 3000000) {     //6s  改成 3s
				key_LONG=1;
				 ESP_LOGI(KEY, "is Key_task KEY_LONG_PRESS= %d !!!\r\n",backup_time);
			}
			else if (backup_time > 2000000) {
				key_MEDIUM=1;
				 ESP_LOGI(KEY, "is Key_task key_MEDIUM= %d !!!\r\n",backup_time);
			} else {
				key_SHORT=1;
				ESP_LOGI(KEY, "is Key_task KEY_SHORT_PRESS= %d !!!\r\n",backup_time);
			}
		}

		if(key_SHORT==1)
		{
			if(System_state.wifi_onoff==0 ) //关闭
			{
				System_state.WIFI_BLEdisplay |=Displayflag_WIFION;  //写便携式显示状态
				IOT_WIFIEvenSetON() ;   	// 按键通知开启WIFI
			}
			if(System_state.ble_onoff==0 ) 		//关闭
			{
				System_state.WIFI_BLEdisplay |=Displayflag_BLEON;   //写便携式显示状态
				IOT_GattsEvenONBLE();   							//按键通知开启BLE
				if(System_state.BLEBindingGg==1)					//ble 绑定设备
			 	System_state.WIFI_BLEdisplay &=~Displayflag_SYSreset;

				IOT_BLEofftime_uc(ESP_WRITE , BLE_CLOSE_TIME_NOT_CONNECTED);   //按键开启后 15分钟 自动关闭
			}
#if test_wifi
			else if(System_state.ble_onoff==1 ) 		//关闭
			{
			 	IOT_GattsEvenOFFBLE();   		// 按键通知开启BLE
			}
#else
			else if(System_state.ble_onoff==1 )  //关闭
		    {
				IOT_SYSRESTTime_uc(ESP_WRITE ,2 );
				System_state.SYS_RestartFlag=1 ;
				System_state.WIFI_BLEdisplay =0;  //写便携式显示状态
		    }

			IOT_KEYSET0X06(56,System_state.WIFI_BLEdisplay );
#endif
		}
//#if test_wifi
//
//		if(key_MEDIUM==1)
//		{
//			if(System_state.ble_onoff==1 ) 		//关闭
//			{
//				IOT_GattsEvenOFFBLE();   		// 按键通知开启BLE
//			}
//		}
//#endif
//		if(key_MEDIUM==1)
//		{
//			 if(System_state.wifi_onoff==0 ) //关闭
//			 {
//				 System_state.wifi_onoff=1;
//				 System_state.WIFI_BLEdisplay |=Displayflag_WIFION;  //写便携式显示状态
//				 IOT_WIFIEvenSetON() ;   	// 按键通知开启WIFI
//			 }
//			 IOT_KEYSET0X06(56,System_state.WIFI_BLEdisplay );
//		}

		if(key_LONG==1)	 //恢复出厂设置
		{
			 System_state.WIFI_BLEdisplay =Displayflag_SYSreset;  //写便携式显示状态
			// System_state.WIFI_BLEdisplay =0;  //写便携式显示状态
			 IOT_KEYSET0X06(56,System_state.WIFI_BLEdisplay );

			 System_state.wifi_onoff = 0 ;  // wifi 即将关闭
			 IOT_KEYlongReset();

			 System_state.SYS_RestartFlag=1;
			 IOT_SYSRESTTime_uc(ESP_WRITE ,2);

		}

		key_SHORT=0;
		key_MEDIUM=0;
		key_LONG=0;

#if DEBUG_HEAPSIZE

    if(timer_logoi++ > 10 )
	{ timer_logoi=0;
		ESP_LOGI(KEY, " Key_task uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
	}
#endif

  }
}

void Key_init(void )
{
	gpio_reset_pin(KEY_PIN);
	gpio_set_direction(KEY_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(KEY_PIN, GPIO_PULLUP_ONLY);
  //gpio_set_intr_type(KEY_PIN, GPIO_INTR_NEGEDGE);    //设置电平下降沿触发中断
    gpio_set_intr_type(KEY_PIN, GPIO_INTR_ANYEDGE);    //任意电平触发

#if Semaphore_Enable
    Key_handle = xSemaphoreCreateBinary();    //创建一个二进制信号量
#else
	keyevt_queue = xQueueCreate(2, sizeof(uint32_t));
#endif
  //添加中断处理函数
    xTaskCreate(Key_task,"Key_task", 1024*3, NULL, configMAX_PRIORITIES-9,NULL);
  //第一个处理器核安装中断处理程序
   gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
   gpio_isr_handler_add(KEY_PIN, Key_isr_handler,  (void*) KEY_PIN);

}



