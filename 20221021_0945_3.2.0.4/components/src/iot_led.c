/*
 * iot_led.c
 *
 *  Created on: 2021��11��16��
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

#define Queue_Enable      0    //����ʹ�ö���
#define Semaphore_Enable  1    //����ʹ�ö�ֵ�ź���


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

	gpio_set_level(LEDBLUE_PIN,0); //��
	gpio_set_level(LEDGREE_PIN,0); // ��
	gpio_set_level(LEDREG_PIN,0); // ��

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


//1�������ƶ���������ʾû�м�⵽����豸
//2���̵���˸�����Ӳ��Ϸ�����
//3��������˸����������
//4����Ƴ���    wifiģ�����  ��ȡflash����      LEDREG_PIN
//5���̵Ƴ���	 �޷����ӵ�·����  				 LEDGREE_PIN
//6�����Ƴ���    BLE ����     				 LEDBLUE_PIN
//7��L  ��   H  ��

void IOT_LED_Polling(void)
{
	static uint8_t led_blinkflag=0;

	if( System_state.SYS_RestartFlag==1)
	{
		gpio_set_level(LEDBLUE_PIN,0); //����
		gpio_set_level(LEDGREE_PIN,0); //����
		gpio_set_level(LEDREG_PIN,0); //����
	}
	else if(System_state.ble_onoff == 1)		//�������� ���Ƴ���
	{
		gpio_set_level(LEDBLUE_PIN,0); //����
		gpio_set_level(LEDGREE_PIN,1); //��
		gpio_set_level(LEDREG_PIN,1); //��
	}
	else if(System_state.Pv_state == 0 )  //û�������Ϲ���豸	 ȫ��
	{
		 gpio_set_level(LEDGREE_PIN,1); //��
		 gpio_set_level(LEDREG_PIN,1); //��
		 gpio_set_level(LEDBLUE_PIN,1); //��
	}
	else if(System_state.SmartConfig_OnOff_Flag == 1 )	 //20220727 chen �ɼ���δ���� �����˸
	{
		led_blinkflag++;
		gpio_set_level(LEDGREE_PIN,1); //��
		gpio_set_level(LEDBLUE_PIN,1); //��
		gpio_set_level(LEDREG_PIN,0); //��-��
	}
	else if(System_state.Wifi_state == 0 )		//δ����·����	//20220727 chen �̵Ƴ������ȼ���ǰ
	{
		gpio_set_level(LEDGREE_PIN,0); //����
		gpio_set_level(LEDBLUE_PIN,1); //��
		gpio_set_level(LEDREG_PIN,1); //��
	}
	else if( System_state.Mbedtls_state==0 )
	{
		led_blinkflag++;
		gpio_set_level(LEDGREE_PIN,0); //��-��
		gpio_set_level(LEDREG_PIN,1);  //��
		gpio_set_level(LEDBLUE_PIN,1); //��
	}
	else if(System_state.Wifi_state == 1 && System_state.Pv_state == 1 && System_state.Mbedtls_state==1 )
	{
		led_blinkflag++;
		gpio_set_level(LEDBLUE_PIN,0); //��-��
		gpio_set_level(LEDREG_PIN,1);  //��
		gpio_set_level(LEDGREE_PIN,1); //��
	}


	if(led_blinkflag>=2)
	{
		led_blinkflag=0;
		//vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(LEDGREE_PIN,1); //��
		gpio_set_level(LEDREG_PIN,1); //��
		gpio_set_level(LEDBLUE_PIN,1); //��
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
	if(bleled_blinkflag>=2)  //��˸
	{
		bleled_blinkflag=0;
		IOT_LED1_OFF(); //��
	}
	if(wifiled_blinkflag>=2)
	{
		wifiled_blinkflag=0;
		IOT_LED2_OFF(); //��
	}

}


//���з�ʽ��������������д����У��ٶ�ȡ����
// ���尴�°�ťö������
typedef enum {
	KEY_SHORT_PRESS = 1,
	KEY_LONG_PRESS,
} alink_key_t;

QueueHandle_t  keyevt_queue =NULL;

//��ֵ�ź���������ĳ��������жϷ���ʱ�����������������൱�����������ж�ͬ����
//�������ж��¼���������Ĺ�����ͬ����������ɣ��жϷ�������(ISR)��ֻ�ǿ��ٴ����ٲ��ֹ�����
//��ֵ�ź�����Ӧ����ͬ��
#if Semaphore_Enable

#define MIN_DELAY 200
static xSemaphoreHandle Key_handle = NULL;

#endif

//��ť�жϴ���
void IRAM_ATTR Key_isr_handler(void* arg)
{
#if Semaphore_Enable
  int last_time = 0;
// ��ȡϵͳ��ǰ���е�ʱ�ӽ�����, �˺����������жϷ�������������, ���������������õĻ���
// ��Ҫʹ�ú��� xTaskGetTickCount, �����������в��ɻ���
  int now = xTaskGetTickCountFromISR();
  if (now - last_time > MIN_DELAY)
  {
     int ignored = pdTRUE;
     //ͨ���ͷ��ź�����ʹ����������
     xSemaphoreGiveFromISR(Key_handle, &ignored);
     last_time = now;
  }
#endif
#if Queue_Enable
  uint32_t gpio_num = (uint32_t) arg;
  xQueueSendFromISR(keyevt_queue, &gpio_num, NULL);  // ���п������ڶ������ �Ѱ���д�����
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
	  //��ȡ�ź�����portMAX_DELAY��ʶһֱ�ȴ����������þ����ʱ�ӵδ����
	    if( xSemaphoreTake( Key_handle,  3500/ portTICK_PERIOD_MS) == pdTRUE )
	 	{
//	      ESP_LOGI(KEY, "is Key_task uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
			if (gpio_get_level(KEY_PIN) == 0) {//��ǰ�͵�ƽ����¼���û����°�����ʱ���
				press_key = pdTRUE;
				backup_time = esp_timer_get_time();
				//�����ǰGPIO�ڵĵ�ƽ�Ѿ���¼Ϊ���£���ʼ��ȥ�ϴΰ��°�����ʱ���
			} else if (press_key) {
				//��¼̧��ʱ���
				release_key = pdTRUE;
				backup_time = esp_timer_get_time() - backup_time;
			}
			else	//��ʱ����
			{

			}
	 	}
 	    else  //������ʱ���Ϳ�
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
	    // ���մ���Ϣ���з�������Ϣ
		xQueueReceive(keyevt_queue, &io_num, portMAX_DELAY);
		ESP_LOGI(KEY, "is Key_task keyevt_queue= %d !!!\r\n", io_num);
		if (gpio_get_level(io_num) == 0) {//��ǰ�͵�ƽ����¼���û����°�����ʱ���
			press_key = pdTRUE;
			backup_time = esp_timer_get_time();
			//�����ǰGPIO�ڵĵ�ƽ�Ѿ���¼Ϊ���£���ʼ��ȥ�ϴΰ��°�����ʱ���
		} else if (press_key) {
			//��¼̧��ʱ���
			release_key = pdTRUE;
			backup_time = esp_timer_get_time() - backup_time;
		}
#endif
		//�������±�־λ�Ͱ��������־λ��Ϊ1ʱ�򣬲�ִ�лص�
		if (press_key & release_key)
		{
			press_key = pdFALSE;
			release_key = pdFALSE;
			//�������1s��ص�����������Ͷ̰��ص�
			if (backup_time > 3000000) {     //6s  �ĳ� 3s
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
			if(System_state.wifi_onoff==0 ) //�ر�
			{
				System_state.WIFI_BLEdisplay |=Displayflag_WIFION;  //д��Яʽ��ʾ״̬
				IOT_WIFIEvenSetON() ;   	// ����֪ͨ����WIFI
			}
			if(System_state.ble_onoff==0 ) 		//�ر�
			{
				System_state.WIFI_BLEdisplay |=Displayflag_BLEON;   //д��Яʽ��ʾ״̬
				IOT_GattsEvenONBLE();   							//����֪ͨ����BLE
				if(System_state.BLEBindingGg==1)					//ble ���豸
			 	System_state.WIFI_BLEdisplay &=~Displayflag_SYSreset;

				IOT_BLEofftime_uc(ESP_WRITE , BLE_CLOSE_TIME_NOT_CONNECTED);   //���������� 15���� �Զ��ر�
			}
#if test_wifi
			else if(System_state.ble_onoff==1 ) 		//�ر�
			{
			 	IOT_GattsEvenOFFBLE();   		// ����֪ͨ����BLE
			}
#else
			else if(System_state.ble_onoff==1 )  //�ر�
		    {
				IOT_SYSRESTTime_uc(ESP_WRITE ,2 );
				System_state.SYS_RestartFlag=1 ;
				System_state.WIFI_BLEdisplay =0;  //д��Яʽ��ʾ״̬
		    }

			IOT_KEYSET0X06(56,System_state.WIFI_BLEdisplay );
#endif
		}
//#if test_wifi
//
//		if(key_MEDIUM==1)
//		{
//			if(System_state.ble_onoff==1 ) 		//�ر�
//			{
//				IOT_GattsEvenOFFBLE();   		// ����֪ͨ����BLE
//			}
//		}
//#endif
//		if(key_MEDIUM==1)
//		{
//			 if(System_state.wifi_onoff==0 ) //�ر�
//			 {
//				 System_state.wifi_onoff=1;
//				 System_state.WIFI_BLEdisplay |=Displayflag_WIFION;  //д��Яʽ��ʾ״̬
//				 IOT_WIFIEvenSetON() ;   	// ����֪ͨ����WIFI
//			 }
//			 IOT_KEYSET0X06(56,System_state.WIFI_BLEdisplay );
//		}

		if(key_LONG==1)	 //�ָ���������
		{
			 System_state.WIFI_BLEdisplay =Displayflag_SYSreset;  //д��Яʽ��ʾ״̬
			// System_state.WIFI_BLEdisplay =0;  //д��Яʽ��ʾ״̬
			 IOT_KEYSET0X06(56,System_state.WIFI_BLEdisplay );

			 System_state.wifi_onoff = 0 ;  // wifi �����ر�
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
  //gpio_set_intr_type(KEY_PIN, GPIO_INTR_NEGEDGE);    //���õ�ƽ�½��ش����ж�
    gpio_set_intr_type(KEY_PIN, GPIO_INTR_ANYEDGE);    //�����ƽ����

#if Semaphore_Enable
    Key_handle = xSemaphoreCreateBinary();    //����һ���������ź���
#else
	keyevt_queue = xQueueCreate(2, sizeof(uint32_t));
#endif
  //����жϴ�����
    xTaskCreate(Key_task,"Key_task", 1024*3, NULL, configMAX_PRIORITIES-9,NULL);
  //��һ���������˰�װ�жϴ������
   gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
   gpio_isr_handler_add(KEY_PIN, Key_isr_handler,  (void*) KEY_PIN);

}



