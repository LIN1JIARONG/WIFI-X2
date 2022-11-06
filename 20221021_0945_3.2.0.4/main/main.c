/*
 * main.c
 *  Created on: 2021��12��1��
 *  Author: Administrator
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/timer.h"

#include "iot_led.h"
#include "iot_scan.h"
#include "iot_spi_flash.h"
#include "iot_station.h"
#include "iot_timer.h"
#include "iot_uart1.h"
//#include "iot_gatts_server.h"
#include "iot_system.h"
#include "iot_rtc.h"
#include "iot_net.h"
#include "iot_inverter.h"
#include "iot_protocol.h"
#include "iot_https_ota.h"
#include "iot_gatts_table.h"
#include "sdkconfig.h"


#include "esp_spi_flash.h"
#include "esp_log.h"
#include "iot_params.h"
#include "iot_fota.h"
#include "iot_record.h"
static const char *TAG = "Main_Init";

extern record_storage_format_param_t GformatParam;

TaskHandle_t BLE_Handle = NULL;
TaskHandle_t WiFi_Handle = NULL;
TaskHandle_t SYS_Handle = NULL;
TaskHandle_t URX_Handle = NULL;
TaskHandle_t UTX_Handle = NULL;
TaskHandle_t Time_Handle = NULL;
TaskHandle_t NetRX_Handle = NULL;
TaskHandle_t NetTX_Handle = NULL;
//TaskHandle_t GetRecord_Handle = NULL;
//TaskHandle_t HTTP_Handle = NULL;


////�û��Զ��������
//�̶���λ��ƫ�� sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t) 24 + 8 + 256 = 288 �ֽ� ֮��
typedef struct {
	char USER_DATA[USER_DATA_LEN];

} __attribute__((packed)) esp_custom_app_desc_t;	//ǿ����1�ֽڵ�ַ����


const __attribute__((section(".rodata_custom_desc"))) esp_custom_app_desc_t custom_app_desc = {.USER_DATA = {DATALOG_TYPE} };


/*
 * 20220708   �޸�HTTP���ر�302���������� �� ����HTTP������Ҫ����֤�����URL ��ҪΪHTTPS��վ
 * 20220708   ���������زɼ����̼���ɺ�У��̼����豸����
 * 20220711   �޸Ĳɼ�������·��������ʾ�����֣���ʽΪ GRT-SN
 *
 * 20220718   �޸���������
 * 20220722   ���ӱ�׼����ģʽ
 */


/**
 * MEMORY_CHECK_TAG   ָ����������ȫ�־�̬��
 * MEMORY_CHECK_TAG   ָ�����ָ���ֵ���ܸı�
 * "memory_check"     �ַ�������������ֳ�����
 */



static const char *MEMORY_CHECK_TAG = "memory_check";

void memory_check_task(void *pvParameter){
        while (1) {
                ESP_LOGW(MEMORY_CHECK_TAG, "%d: - INTERNAL RAM left %dKB", __LINE__,
                                heap_caps_get_free_size( MALLOC_CAP_INTERNAL )/1024);
                ESP_LOGW(MEMORY_CHECK_TAG, "%d: - SPI      RAM left %dkB", __LINE__,
                                heap_caps_get_free_size( MALLOC_CAP_SPIRAM )/1024);

                ESP_LOGW(MEMORY_CHECK_TAG, "minimum_free_heap_size = %d " ,esp_get_minimum_free_heap_size());
                ESP_LOGW(MEMORY_CHECK_TAG, "esp_get_free_heap_size = %d",  esp_get_free_heap_size());
                vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
        vTaskDelete(NULL);
}

void thread_check_func(const char *thread_name, int thread_line) {
        ESP_LOGW(thread_name, "%d: - Thread\tdepth left %d", thread_line,
                        uxTaskGetStackHighWaterMark(NULL));
}

void IOT_SYSTEM_Init(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();			//��ʼ��Ĭ�� NVS ������
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    System_state.uiSystemInitStatus = 0;

    IOT_GPIO_Init();
    IOT_UART0_Init();

#if UART1_EVENTS
   IOT_UART1_events_init();
   xTaskCreate(IOT_ModbusRXeventsTask , "ModbusRXTask" , 1024*3 , NULL, configMAX_PRIORITIES - 1, &URX_Handle);
#else
    IOT_UART1_Init();
    xTaskCreate(IOT_ModbusRXTask , "ModbusRXTask", 1024*3, NULL, configMAX_PRIORITIES-1, &URX_Handle);
#endif
   xTaskCreate(IOT_ModbusTXTask, "ModbusTXTask", 1024*3, NULL, configMAX_PRIORITIES - 2, &UTX_Handle);
   IOT_DTCInit(0); 				// Ĭ���ֶγ�ʼ��

   if(IOT_ESP_InverterType_Get()==1)  //��ȡDTC
   IOT_ESP_InverterSN_Get();		//��ȡ���к�

   IOT_InCMDTime_uc(ESP_WRITE , 1);
   System_state.WIFI_BLEdisplay = 0;
   System_state.Pv_state = 0;
   InvCommCtrl.CMDFlagGrop = 0;
   InvCommCtrl.CMDCur = HOLDING_GROP;

   IOT_Inverter_RTData_TimeOut_Check_uc(ESP_WRITE , INVERTER_RT_DATA_TIMEOUT);  //����ʵʱ���ݳ�ʱʱ��
   IOT_WiFiConnect_Server_Timeout_uc(ESP_WRITE , CONNECT_SERVER_TIMEOUT);	 // �������ӷ�������ʱʱ��

   printf("\r\n\r\n--------VERSION : %s 20220917 ----------\r\n\r\n",DATALOG_SVERSION);

#if (test_wifi && 1)
	uint8_t intdelay=0;
	intdelay=5;
	while(intdelay-- )   // �ϵ���ʱ30 ��  led����30s
	{
	    vTaskDelay(1000 / portTICK_PERIOD_MS);
	    ESP_LOGI(TAG, "IOT_System_ConfigInit  vTaskDelay intdelay =%d   DTC =0x%x\r\n",intdelay,InvCommCtrl.DTC );
	    if( InvCommCtrl.DTC == 0xFEFE)
	    {
	     	break ;
	    }
	}
	gpio_set_level(LEDBLUE_PIN,0); //��
	gpio_set_level(LEDGREE_PIN,0); // ��
	gpio_set_level(LEDREG_PIN,0); // ��
#endif


	IOT_SPI_FLASH_Init();
	ESP_LOGI(TAG, "IOT_System_ConfigInit vTaskDelay OK\r\n");

	IOT_SystemFunc_Init();		// ϵͳ���ܳ�ʼ��
	IOT_AddParam_Init();

	IOT_Reset_Reason_Printf();

	System_state.uiSystemInitStatus = 1;	//ϵͳ��ʼ�����,Flag ��1

}

void app_main(void)
{
    IOT_SYSTEM_Init();
    IOT_timer_Init();
    IOT_CLOCK_SOFTInit();
    IOT_GETMAC();

	#if test_wifi
    IOT_ReadRecord_Init();
	#endif
//  RTOS ISR stack size [1536,327680]     1536 �ֽ� ��ʱ�� 0
//  ����������
    xTaskCreate(IOT_GattsTable_task , "Gattstask" , 1024 * 5, NULL, configMAX_PRIORITIES - 1, &BLE_Handle);
//  wifi����
    xTaskCreate(IOT_WIFIstation_Task , "IOT_WIFITask" , 1024 * 5, NULL, configMAX_PRIORITIES-3, &WiFi_Handle);
//  ϵͳ��������
    xTaskCreate(IOT_SysTEM_Task , "SysTEMTask", 1024*3, NULL, configMAX_PRIORITIES - 8, &SYS_Handle);
//  ��ʱ��
    xTaskCreate(IOT_Timer_task , "Timertask", 1024*3, NULL, configMAX_PRIORITIES - 5, &Time_Handle);
//  ����������


    IOT_KEYSET0X06(56, System_state.WIFI_BLEdisplay);
    Key_init();
    xTaskCreate(IOT_MbedtlsRead_task,"MbedtlsRtask", 1024*6, NULL, configMAX_PRIORITIES - 6, &NetRX_Handle);	//���ȼ���ֵԽС����ô����������ȼ�Խ�ͣ�������������ȼ���0
    xTaskCreate(IOT_MbedtlsWrite_task,"MbedtlsWtask", 1024*6, NULL, configMAX_PRIORITIES - 7, &NetTX_Handle);
   // xTaskCreate(memory_check_task,"memory_check_task", 1024*4, NULL, configMAX_PRIORITIES-1, NULL);
    ESP_LOGI(TAG, "is main uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
#if test_wifi
    IOT_WiFistationInit();
#endif

    int i = 0;
#if 0
  int j = 0;
  uint8_t pbuff[10 + 6 + 750] = {0};
  uint8_t *readbuff = (uint8_t *)malloc(1024);
  bzero(readbuff , 1024);
  uint16_t readLen = 0;
#endif
    while(1)
    {
     //ESP_LOGI(TAG, " 03HoldingSentFlag=%d ,updatetimeflag%d  Up04dateTime = %d  MqttDATA_Time=%d!!!\r\n",InvCommCtrl.HoldingSentFlag ,System_state.updatetimeflag,System_state.Up04dateTime,Mqtt_state.MqttDATA_Time);
     //ESP_LOGI(TAG, " main uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
     //ESP_LOGI(TAG, " main minimum free heap size = %d !!!\r\n", esp_get_minimum_free_heap_size());
     //ESP_LOGI(TAG, " esp_get_free_heap_size size = %d !!!\r\n", esp_get_free_heap_size());
     //ESP_LOGI(TAG, "System_state.WIFI_BLEdisplay = %x !!!\r\n", System_state.WIFI_BLEdisplay);
     //ESP_LOGI(TAG, "System_state.Mbedtls_state  =%d System_state.Wifi_state=%d",System_state.Mbedtls_state,System_state.Wifi_state);
     //ESP_LOGI(TAG, "System_state.Mbedtls_state  =%d System_state.MQTT_state=%d",System_state.Mbedtls_state,System_state.MQTT_state);
     //ESP_LOGI(TAG, "System_state.BLEKeysflag    =%d System_state.Ble_state=%d",System_state.BLEKeysflag,System_state.Ble_state);
	 //ESP_LOGI(TAG," Mqtt_state.Historydate_Time=%d ,System_state.upHistorytimeflag=%d  \r\n",Mqtt_state.Historydate_Time,System_state.upHistorytimeflag);
		if(++i >= 20)
		{
			i = 0;
//	 		IOT_Printf("System_state.ble_onoff = %d\r\n",System_state.ble_onoff);
//	 		IOT_Printf("System_state.wifi_onoff = %d\r\n",System_state.wifi_onoff);
//	 		IOT_Printf("System_state.Ble_state = %d\r\n",System_state.Ble_state);
//	 		IOT_Printf("System_state.Wifi_state = %d\r\n",System_state.Wifi_state);
//	 		IOT_Printf("System_state.SSL_state = %d\r\n",System_state.SSL_state);
//			IOT_Printf("_g_EMSGMQTTSendTCPType = %d\r\n", _g_EMSGMQTTSendTCPType);
//			IOT_Printf("Mqtt_state.MQTT_connet = %d\r\n",Mqtt_state.MQTT_connet);
//			IOT_Printf("System_state.Mbedtls_state = %d\r\n",System_state.Mbedtls_state);
//			IOT_Printf("g_FotaParams.cMasterFotaflag = %d\r\n",g_FotaParams.cMasterFotaflag);
//			IOT_Printf("g_FotaParams.uiFotaProgress = %d\r\n",g_FotaParams.uiFotaProgress);
//			IOT_Printf("g_FotaParams.uiWayOfUpdate = %d\r\n",g_FotaParams.uiWayOfUpdate);
//			IOT_Printf("g_FotaParams.ProgressType = 0x%x\r\n",g_FotaParams.ProgressType);
//			IOT_Printf("System_state.Wifi_ActiveReset = 0x%x \r\n" , System_state.Wifi_ActiveReset);
//			IOT_Printf("System_state.upHistorytimeflag = %d \r\n",System_state.upHistorytimeflag);
//			IOT_Printf("System_state.Pv_state = %d \r\n",System_state.Pv_state);
//			IOT_Printf("System_state.Server_Online = %d \r\n",System_state.Server_Online);
//			IOT_Printf("GformatParam.uiPacketSize = %d \r\n",GformatParam.uiPacketSize);
#ifdef Kennedy_main
			IOT_Printf("Mqtt_state.MqttSend_step = %d\r\n",Mqtt_state.MqttSend_step);
			IOT_Printf("InvCommCtrl.HoldingSentFlag = %d\r\n",InvCommCtrl.HoldingSentFlag);
			IOT_Printf("Mqtt_state.MqttPING_Time = %d\r\n",Mqtt_state.MqttPING_Time);
			IOT_Printf("Mqtt_state.Heartbeat_Timeout = %d\r\n",Mqtt_state.Heartbeat_Timeout);
			IOT_Printf("InvCommCtrl.ContrastInvTimeFlag = %d\r\n",InvCommCtrl.ContrastInvTimeFlag);
			IOT_Printf("InvCommCtrl.CMDFlagGrop & CMD_0X10 = 0x%x\r\n",InvCommCtrl.CMDFlagGrop & CMD_0X10);
			IOT_Printf("_GucInvPollingStatus= %d \r\n",_GucInvPollingStatus);
#endif
		}

		vTaskDelay(100  / portTICK_PERIOD_MS);

    }
}
